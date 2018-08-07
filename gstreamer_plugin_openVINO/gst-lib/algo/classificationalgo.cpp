
/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <string>
#include "classificationalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace std;
//using namespace HDDLStreamFilter;

static std::string g_vehicleLabel[] =
{
#include "label-en.txt"
};


#define CLASSIFICATION_OBJECT_FLAG_DONE 0x10
#define CLASSIFICATION_OBJECT_FLAG_VALID 0x100


#define CLASSIFICATION_INPUT_W 448
#define CLASSIFICATION_INPUT_H 448


static void try_process_algo_data(CvdlAlgoData *algoData)
{
    bool allObjDone = true;
    ClassificationAlgo *classificationAlgo = static_cast<ClassificationAlgo*>(algoData->algoBase);

    // check if this frame has been done, which contains multiple objects
    for(unsigned int i=0; i<algoData->mObjectVec.size();i++)
        if(!(algoData->mObjectVec[i].flags & CLASSIFICATION_OBJECT_FLAG_DONE))
            allObjDone = false;

    // if all objects are done, then push it into output queue
    if(allObjDone) {
        // delete invalid objects
        // remove the objectData which has not been set VALID flag
        std::vector<ObjectData> &objectVec = algoData->mObjectVec;
        std::vector<ObjectData>::iterator obj;
        for (obj = objectVec.begin(); obj != objectVec.end();) {
            if(!((*obj).flags & CLASSIFICATION_OBJECT_FLAG_VALID)) {
                // remove  it
                obj = objectVec.erase(obj);
                continue;
            } else {
                ++obj;
            }
        }
        if(objectVec.size()>0) {
            //put valid algoData;
            classificationAlgo->mOutQueue.put(*algoData);
        } else {
            delete algoData;
        }
    }
}

static void process_one_object(CvdlAlgoData *algoData, ObjectData &objectData, int objId)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *ocl_buf = NULL;
    ClassificationAlgo *classificationAlgo = static_cast<ClassificationAlgo*>(algoData->algoBase);
 
    VideoRect crop = { (uint32_t)objectData.rect.x, (uint32_t)objectData.rect.y,
                       (uint32_t)objectData.rect.width, (uint32_t)objectData.rect.height};
    if(crop.width<=0 || crop.height<=0 || crop.x<0 || crop.y<0) {
        GST_ERROR("classfication: crop = (%d,%d) %dx%d", crop.x, crop.y, crop.width, crop.height);
        objectData.flags |= CLASSIFICATION_OBJECT_FLAG_DONE;
        //objectData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_DONE);
        try_process_algo_data(algoData);
        return;
    }
    classificationAlgo->mImageProcessor.process_image(algoData->mGstBuffer,&ocl_buf,&crop);
    if(ocl_buf==NULL) {
        GST_WARNING("Failed to do image process!");
        objectData.flags |= CLASSIFICATION_OBJECT_FLAG_DONE;
        //objectData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_DONE);
        try_process_algo_data(algoData);
        return;
    }
    //algoData->mGstBufferOcl = ocl_buf;
    objectData.oclBuf = ocl_buf;

    OclMemory *ocl_mem = NULL;
    ocl_mem = ocl_memory_acquire (ocl_buf);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        if(ocl_buf)
            gst_buffer_unref(ocl_buf);
        objectData.flags |= CLASSIFICATION_OBJECT_FLAG_DONE;
        //objectData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_DONE);
        try_process_algo_data(algoData);
        return;
    }

    // Classification callback function
    auto onClassificationResult = [&objectData](CvdlAlgoData* &algoData)
    {
        ClassificationAlgo *classificationAlgo = static_cast<ClassificationAlgo *>(algoData->algoBase);
        // this ocl will not use, free it here
        gst_buffer_unref(objectData.oclBuf);

        classificationAlgo->mInferCnt--;
        objectData.flags |= CLASSIFICATION_OBJECT_FLAG_DONE;
        //objectData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_DONE);
        // check and process algoData
        try_process_algo_data(algoData);
    };

    // ASync detect, directly return after pushing request.
    ret = classificationAlgo->mIeLoader.do_inference_async(algoData, algoData->mFrameId,objId,
                                                        ocl_mem->frame, onClassificationResult);
    classificationAlgo->mInferCnt++;
    classificationAlgo->mInferCntTotal++;

    if (ret!=GST_FLOW_OK) {
        GST_ERROR("IE: detect FAIL");
    }
    return;
}

/*
 * This is the main function of cvdl task:
 *     1. NV12 --> BGR_Planar
 *     2. async inference
 *     3. parse inference result and put it into in_queue of next algo
 */
static void classification_algo_func(gpointer userData)
{
    ClassificationAlgo *classificationAlgo = static_cast<ClassificationAlgo*> (userData);
    CvdlAlgoData *algoData = new CvdlAlgoData;
    //GstFlowReturn ret;

    if(!classificationAlgo->mNext) {
        GST_LOG("The classification algo's next algo is NULL");
    }

    if(!classificationAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(classificationAlgo);

    for(unsigned int i=0; i< algoData->mObjectVec.size(); i++) {
        algoData->mObjectVec[i].flags &= ~(CLASSIFICATION_OBJECT_FLAG_DONE|CLASSIFICATION_OBJECT_FLAG_VALID);
        //algoData->mObjectVec[i].flags.fetch_and(~(CLASSIFICATION_OBJECT_FLAG_DONE|CLASSIFICATION_OBJECT_FLAG_VALID));
    }

    // get input data and process it here, put the result into algoData
    // NV12-->BGR_Plannar
    for(unsigned int i=0; i< algoData->mObjectVec.size(); i++)
        process_one_object(algoData, algoData->mObjectVec[i], i);


    /**
        * Save current images to 'detectPoolTrck' that will be used for tracking.
        * Detect result will be saved to '_detectRsltPool' in the callback function ''.
        */
    //global->detectPoolTrck.put(DetectObjRslt { img, bDetect });

}

ClassificationAlgo::ClassificationAlgo() : CvdlAlgoBase(classification_algo_func, this, NULL)
{
    //mImageProcessor.set_ocl_kernel_name(CRC_FORMAT_BGR_PLANNAR);
    mInputWidth = CLASSIFICATION_INPUT_W;
    mInputHeight = CLASSIFICATION_INPUT_H;
}


ClassificationAlgo::~ClassificationAlgo()
{

}

void ClassificationAlgo::set_data_caps(GstCaps *incaps)
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    if(mOclCaps)
        gst_caps_unref(mOclCaps);

    //int oclSize = mInputWidth * mInputHeight * 3;
    mOclCaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    gst_caps_set_simple (mOclCaps, "width", G_TYPE_INT, mInputWidth, "height",
      G_TYPE_INT, mInputHeight, NULL);

    mImageProcessor.ocl_init(incaps, mOclCaps, IMG_PROC_TYPE_OCL_CRC, CRC_FORMAT_BGR_PLANNAR);
    mImageProcessor.get_input_video_size(&mImageProcessorInVideoWidth,
                                         &mImageProcessorInVideoHeight);
    //gst_caps_unref (mOclCaps)
    //return GST_FLOW_OK;
}

GstFlowReturn ClassificationAlgo::algo_dl_init(char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    ret = mIeLoader.set_device(InferenceEngine::TargetDevice::eHDDL);
    if(ret != GST_FLOW_OK){
        GST_ERROR("IE failed to set device be eHDDL!");
        return GST_FLOW_ERROR;
    }

    // Load different Model based on different device.
    std::string strModelXml(modeFileName);
    std::string tmpFn = strModelXml.substr(0, strModelXml.rfind("."));
    std::string strModelBin = tmpFn + ".bin";
    GST_DEBUG("ClassificationModel bin = %s", strModelBin.c_str());
    GST_DEBUG("ClassificationModel xml = %s", strModelXml.c_str());
    ret = mIeLoader.read_model(strModelXml, strModelBin, IE_MODEL_CLASSFICATION);
    
    if(ret != GST_FLOW_OK){
        GST_ERROR("IELoder failed to read model!");
        return GST_FLOW_ERROR;
    }
    return ret;
}


GstFlowReturn ClassificationAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                  int precision, CvdlAlgoData *outData, int objId)
{
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);
    //float *input = static_cast<float*>(resultBlobFp32->data());

    // Get top N results
    std::vector<unsigned> topIndexes;
    int topnum = 1;
    ObjectData &objData = outData->mObjectVec[objId];
    InferenceEngine::TopResults(topnum, *resultBlobFp32, topIndexes);
    if (topIndexes.size()) {
        for (size_t i = 0; i < topIndexes.size(); i++) {
            float prob = (resultBlobFp32->data()[topIndexes[i]]);
            std::string strLabel = g_vehicleLabel[topIndexes[i]];
            objData.prob = prob;
            objData.label = strLabel;
            objData.objectClass =  topIndexes[i];
            objData.flags |= CLASSIFICATION_OBJECT_FLAG_VALID;
            //objData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_VALID);
            break;
        }
    }

    return GST_FLOW_OK;
}

// dequeue a buffer with cvdlMeta data
GstBuffer* ClassificationAlgo::dequeue_buffer()
{
    GstBuffer* buf = NULL;
    CvdlAlgoData algoData;
    gpointer meta_data;

    while(true) {
        if(!mOutQueue.get(algoData)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return NULL;
        }
        if(algoData.mObjectVec.size()>0)
            break;
    }
    buf = algoData.mGstBuffer;
    // put object data as meta data

    VASurfaceID surface;
    VADisplay display;
    unsigned int color = 0x00FF00;
    surface= gst_get_mfx_surface (buf, NULL, &display);

    for(unsigned int i=0; i<algoData.mObjectVec.size(); i++) {
        VideoRect rect;
        rect.x     = algoData.mObjectVec[i].rect.x;
        rect.y     = algoData.mObjectVec[i].rect.y;
        rect.width = algoData.mObjectVec[i].rect.width;
        rect.height= algoData.mObjectVec[i].rect.height;
        if(i==0)
            meta_data = cvdl_meta_create(display, surface, &rect, algoData.mObjectVec[i].label.c_str(), color);
        else
            cvdl_meta_add (meta_data, &rect, algoData.mObjectVec[i].label.c_str(), color);
    }

    gst_buffer_set_cvdl_meta(buf, (CvdlMeta *)meta_data);
    return buf;
}

