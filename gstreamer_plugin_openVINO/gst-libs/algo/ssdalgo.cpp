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
#include "ssdalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace HDDLStreamFilter;
using namespace std;


const char* VOC_LABEL_MAPPING[] = {
    "background",    "aeroplane",    "bicycle",    "bird",    "boat",    "bottle",
    "bus",    "car",    "cat",    "chair",    "cow",    "diningtable",    "dog",    "horse",
    "motorbike",    "person",    "pottedplant",    "sheep",    "sofa",    "train",
    "tvmonitor"
};

const std::vector<std::string> TRACKING_CLASSES = {
    "person",    "bus",    "car"
};

/*
 * This is the main function of cvdl task:
 *     1. NV12 --> BGR_Planar
 *     2. async inference
 *     3. parse inference result and put it into in_queue of next algo
 */
static void ssd_algo_func(gpointer userData)
{
    SSDAlgo *ssdAlgo = static_cast<SSDAlgo*> (userData);
    CvdlAlgoData *algoData = new CvdlAlgoData;
    GstFlowReturn ret;
    gint64 start, stop;

    GST_LOG("\nssd_algo_func - new an algoData = %p\n", algoData);
    if(!ssdAlgo->mNext) {
        GST_WARNING("The next algo is NULL, return");
        return;
    }

    if(!ssdAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(ssdAlgo);
    GST_LOG("%s() - ssdAlgo = %p, algoData->mFrameId = %ld\n",
            __func__, ssdAlgo, algoData->mFrameId);
    GST_LOG("%s() - get one buffer, GstBuffer = %p, refcout = %d, "\
        "queueSize = %d, algoData = %p, algoBase = %p\n",
        __func__, algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT (algoData->mGstBuffer),
        ssdAlgo->mInQueue.size(), algoData, algoData->algoBase);

    // get input data and process it here, put the result into algoData
    // NV12-->BGR_Plannar
    GstBuffer *ocl_buf = NULL;
    VideoRect crop = {0,0, (unsigned int)ssdAlgo->mImageProcessorInVideoWidth,
                           (unsigned int)ssdAlgo->mImageProcessorInVideoHeight};
    start = g_get_monotonic_time();
    ssdAlgo->mImageProcessor.process_image(algoData->mGstBuffer,NULL, &ocl_buf, &crop);
    stop = g_get_monotonic_time();
    ssdAlgo->mImageProcCost += (stop - start);

    if(ocl_buf==NULL) {
        GST_WARNING("Failed to do image process!");
        g_print("ssd_algo_func - Failed to do image process!\n");
        return;
    }
    // No crop, the whole frame was be resized to it
    algoData->mGstBufferOcl = ocl_buf;

    OclMemory *ocl_mem = NULL;
    ocl_mem = ocl_memory_acquire (ocl_buf);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        return;
    }

    //test
    //ssdAlgo->save_buffer(ocl_mem->frame.getMat(0).ptr(), ssdAlgo->mInputWidth,
    //                   ssdAlgo->mInputHeight, 3,algoData->mFrameId, 1, "ssd");

    // Detect callback function
    auto onDetectResult = [](CvdlAlgoData* algoData)
    {
        //SSDAlgo *ssdAlgo = static_cast<SSDAlgo *>(algoData->algoBase);
        CvdlAlgoBase *algo = algoData->algoBase;
        GST_LOG("onDetectResult - ssd  algo = %p, OclBuffer = %p(%d)\n", algo, algoData->mGstBufferOcl,
            GST_MINI_OBJECT_REFCOUNT(algoData->mGstBufferOcl));
        gst_buffer_unref(algoData->mGstBufferOcl);

        if(algoData->mResultValid){
            GST_LOG("ssd_algo_func - pass down GstBuffer = %p(%d)\n",
                algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
            algo->mNext->mInQueue.put(*algoData);
        }else{
            //g_print("delete algoData = %p\n", algoData);
            GST_LOG("ssd_algo_func - unref GstBuffer = %p(%d)\n", 
                algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
            gst_buffer_unref(algoData->mGstBuffer);
            delete algoData;
        }
        algo->mInferCnt--;
    };

    start = g_get_monotonic_time();
    // ASync detect, directly return after pushing request.
    ret = ssdAlgo->mIeLoader.do_inference_async(
            algoData, algoData->mFrameId, -1, ocl_mem->frame, onDetectResult);
    stop = g_get_monotonic_time();
    ssdAlgo->mInferCost += (stop - start);

    ssdAlgo->mInferCnt++;
    ssdAlgo->mInferCntTotal++;
    ssdAlgo->mFrameDoneNum++;

    if (ret!=GST_FLOW_OK) {
        GST_ERROR("IE: detect FAIL");
    }
}


SSDAlgo::SSDAlgo() : CvdlAlgoBase(ssd_algo_func, this, NULL)
{
    set_default_label_name();

    //mImageProcessor.set_ocl_kernel_name(CRC_FORMAT_BGR_PLANNAR);
    mInputWidth = 0;
    mInputHeight = 0;
    mIeInited = false;

    mInCaps = NULL;
}

SSDAlgo::~SSDAlgo()
{
    wait_work_done();
    if(mInCaps)
        gst_caps_unref(mInCaps);
    g_print("SSDAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

void SSDAlgo::set_default_label_name()
{
        set_label_names(VOC_LABEL_MAPPING);
}

void SSDAlgo::set_label_names(const char** label_names)
{
    mLabelNames = label_names;
}

void SSDAlgo::set_data_caps(GstCaps *incaps)
{
    // load IE and cnn model
    std::string filenameXML;
    const gchar *env = g_getenv("CVDL_SSD_MODEL_FULL_PATH");
    if(env){
        filenameXML = std::string(env);
    }else{
        filenameXML = std::string(CVDL_MODEL_DIR_DEFAULT"/ssd_detect/MobileNetSSD.xml");
    }
    algo_dl_init(filenameXML.c_str());

    //get data size of ie input
    GstFlowReturn ret = GST_FLOW_OK;
    int w, h, c;
    ret = mIeLoader.get_input_size(&w, &h, &c);

    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    if(ret==GST_FLOW_OK) {
        g_print("SSDAlgo: parse out the input size whc= %dx%dx%d\n", w, h, c);
        mInputWidth = w;
        mInputHeight = h;
    }

    //int oclSize = mInputWidth * mInputHeight * 3;
    mOclCaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    gst_caps_set_simple (mOclCaps, "width", G_TYPE_INT, mInputWidth, "height",
                         G_TYPE_INT, mInputHeight, NULL);

    mImageProcessor.ocl_init(incaps, mOclCaps, IMG_PROC_TYPE_OCL_CRC, CRC_FORMAT_BGR_PLANNAR);
    mImageProcessor.get_input_video_size(&mImageProcessorInVideoWidth,
                                         &mImageProcessorInVideoHeight);
    gst_caps_unref (mOclCaps);

}

GstFlowReturn SSDAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;
    if(mIeInited)
        return ret;
    mIeInited = true;

    ret = mIeLoader.set_device(InferenceEngine::TargetDevice::eMYRIAD);
    if(ret != GST_FLOW_OK){
        GST_ERROR("IE failed to set device be eHDDL!");
        return GST_FLOW_ERROR;
    }
    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(127.0f,  1.0/127.0f);

    // Load different Model based on different device.
    std::string strModelXml(modeFileName);
    std::string tmpFn = strModelXml.substr(0, strModelXml.rfind("."));
    std::string strModelBin = tmpFn + ".bin";
    GST_DEBUG("DetectModel bin = %s", strModelBin.c_str());
    GST_DEBUG("DetectModel xml = %s", strModelXml.c_str());
    ret = mIeLoader.read_model(strModelXml, strModelBin, IE_MODEL_SSD);
    
    if(ret != GST_FLOW_OK){
        GST_ERROR("IELoder failed to read model!");
        return GST_FLOW_ERROR;
    }
    mIeLoader.get_out_size(&mSSDObjectSize, &mSSDMaxProposalCount);
    if (mSSDObjectSize != 7) {
            GST_ERROR("SSD object item should have 7 as a last dimension");
            return GST_FLOW_ERROR;
   }
    return ret;
}

bool SSDAlgo::get_result(float * box,CvdlAlgoData* &outData)
{
     int objectNum = 0;
    outData->mObjectVec.clear();
    for (int i = 0; i < mSSDMaxProposalCount; i++) {
        float image_id = box[i * mSSDObjectSize + 0];

        if (image_id < 0 /* better than check == -1 */) {
            break;
        }

        auto label = (int)box[i * mSSDObjectSize + 1];
        float confidence = box[i * mSSDObjectSize + 2];

        if (confidence < 0.2) {
            continue;
        }

       const char * labelName = mLabelNames[label];
        if(std::find(TRACKING_CLASSES.begin(), TRACKING_CLASSES.end(), labelName) == TRACKING_CLASSES.end()){
            continue;
        }

        auto xmin = (int)(box[i * mSSDObjectSize + 3] * (float)mImageProcessorInVideoWidth);
        auto ymin = (int)(box[i * mSSDObjectSize + 4] * (float)mImageProcessorInVideoHeight);
        auto xmax = (int)(box[i * mSSDObjectSize + 5] * (float)mImageProcessorInVideoWidth);
        auto ymax = (int)(box[i * mSSDObjectSize + 6] * (float)mImageProcessorInVideoHeight);

        if(xmin < 0) xmin = 0; if(xmin >= mImageProcessorInVideoWidth) xmin = mImageProcessorInVideoWidth - 1;
        if(xmax < 0) xmax = 0; if(xmax >= mImageProcessorInVideoWidth) xmax = mImageProcessorInVideoWidth - 1;
        if(ymin < 0) ymin = 0; if(ymin >= mImageProcessorInVideoHeight) ymin = mImageProcessorInVideoHeight - 1;
        if(ymax < 0) ymax = 0; if(ymax >= mImageProcessorInVideoHeight) ymax = mImageProcessorInVideoHeight - 1;

        if(xmin >= xmax || ymin >= ymax) continue;

        // only detect the lower 3/2 part in the image
        if (ymin < (int)((float)mImageProcessorInVideoHeight * 0.1f)) continue;
        if (ymax >= mImageProcessorInVideoHeight -10 ) continue;

        ObjectData object;
        object.id = objectNum;
        object.objectClass = label;
        object.label = std::string(labelName);
        object.prob = confidence;
        object.rect = cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
        object.rectROI = cv::Rect(-1,-1,-1,-1); //for License Plate
        objectNum++;
 
        outData->mObjectVec.push_back(object);
        GST_LOG("SSD %ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n",outData->mFrameId, i,
            object.prob, object.label.c_str(), xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
    }
    if(objectNum>=1){
        // put objectVec into output_queue
        outData->mResultValid = true;
    } else {
        outData->mResultValid = false;
    }
    return true;
}

GstFlowReturn SSDAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("SSDAlgo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }
    get_result(input, outData);

    return GST_FLOW_OK;
}
    
