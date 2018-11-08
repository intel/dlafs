
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
#include "lprecognize.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace std;

const char * NAME_MAPPING[] = {
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "Anhui",
        "Beijing",
        "Chongqing",
        "Fujian",
        "Gansu",
        "Guangdong",
        "Guangxi",
        "Guizhou",
        "Hainan",
        "Hebei",
        "Heilongjiang",
        "Henan",
        "HongKong",
        "Hubei",
        "Hunan",
        "InnerMongolia",
        "Jiangsu",
        "Jiangxi",
        "Jilin",
        "Liaoning",
        "Macau",
        "Ningxia",
        "Qinghai",
        "Shaanxi",
        "Shandong",
        "Shanghai",
        "Shanxi",
        "Sichuan",
        "Tianjin",
        "Tibet",
        "Xinjiang",
        "Yunnan",
        "Zhejiang",
        "police",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z"};

#define LPR_ROWS 71
#define LPR_COLS 88

#define LP_RECOGNIZE_FLAG_DONE 0x10
#define LP_RECOGNIZE_FLAG_VALID 0x100

#define THRESHOLD_PROB 0.2

#define LP_INPUT_W 94
#define LP_INPUT_H 24


static void try_process_algo_data(CvdlAlgoData *algoData)
{
    bool allObjDone = true;
    LpRecognizeAlgo *lprecognizeAlgo = static_cast<LpRecognizeAlgo*>(algoData->algoBase);

    // check if this frame has been done, which contains multiple objects
    for(unsigned int i=0; i<algoData->mObjectVec.size();i++)
        if(!(algoData->mObjectVec[i].flags & LP_RECOGNIZE_FLAG_DONE))
            allObjDone = false;

    // if all objects are done, then push it into output queue
    if(allObjDone) {
        // delete invalid objects
        // remove the objectData which has not been set VALID flag
        std::vector<ObjectData> &objectVec = algoData->mObjectVec;
        std::vector<ObjectData>::iterator obj;
        for (obj = objectVec.begin(); obj != objectVec.end();) {
            LicencePlateData lpData((*obj).label, (*obj).prob, (*obj).rectROI);
            #if 0
            if(!(lprecognizeAlgo->lpPool.check(lpData)))
                 (*obj).flags &=  ~LP_RECOGNIZE_FLAG_VALID;
            #endif
            if(!((*obj).flags & LP_RECOGNIZE_FLAG_VALID)) {
                // remove  it
                obj = objectVec.erase(obj);
                continue;
            } else {
                ++obj;
            }
        }

        //seperate rect and ROI
        std::vector<ObjectData> objectVecCp = algoData->mObjectVec;
        ObjectData objItem;
        algoData->mObjectVec.clear();
        for(unsigned int i=0; i<objectVecCp.size();i++) {
             objItem =  objectVecCp[i];
            objItem.label=std::string("vehicle");
            objItem.prob = 1.0;
            algoData->mObjectVec.push_back(objItem);//car

            objItem =  objectVecCp[i];
            objItem.rect = objItem.rectROI;
            algoData->mObjectVec.push_back(objItem); //LP
        }

        if(objectVec.size()>0) {
            lprecognizeAlgo->lpPool.update();
            //put valid algoData;
            GST_LOG("lp recognize algo - output GstBuffer = %p(%d)\n",
                   algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
            lprecognizeAlgo->mNext->mInQueue.put(*algoData);
        } else {
            GST_LOG("lp recognize algo - unref GstBuffer = %p(%d)\n",
                algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
            gst_buffer_unref(algoData->mGstBuffer);
            delete algoData;
        }
    }
}

static void process_one_object(CvdlAlgoData *algoData, ObjectData &objectData, int objId)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *ocl_buf = NULL;
    LpRecognizeAlgo *lprecognizeAlgo = static_cast<LpRecognizeAlgo*>(algoData->algoBase);
    gint64 start, stop;

    cv::Rect &rectROI = objectData.rectROI;

    // LP recognize model will use the LP to do inference
    VideoRect crop = { (uint32_t)rectROI.x, 
                       (uint32_t)rectROI.y,
                       (uint32_t)rectROI.width,
                       (uint32_t)rectROI.height};

    if(crop.width<=0 || crop.height<=0 || crop.x<0 || crop.y<0) {
        GST_ERROR("lp recognize: crop = (%d,%d) %dx%d", crop.x, crop.y, crop.width, crop.height);
        objectData.flags |= LP_RECOGNIZE_FLAG_DONE;
        try_process_algo_data(algoData);
        return;
    }
    start = g_get_monotonic_time();
    lprecognizeAlgo->mImageProcessor.process_image(algoData->mGstBuffer,NULL,&ocl_buf,&crop);
    stop = g_get_monotonic_time();
    lprecognizeAlgo->mImageProcCost += stop - start;
    if(ocl_buf==NULL) {
        GST_WARNING("Failed to do image process!");
        objectData.flags |= LP_RECOGNIZE_FLAG_DONE;
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
        objectData.flags |= LP_RECOGNIZE_FLAG_DONE;
        //objectData.flags.fetch_or(CLASSIFICATION_OBJECT_FLAG_DONE);
        try_process_algo_data(algoData);
        return;
    }

    #if 0
    //test
    lprecognizeAlgo->save_buffer(ocl_mem->frame.getMat(0).ptr(), lprecognizeAlgo->mInputWidth,
        lprecognizeAlgo->mInputHeight,3,algoData->mFrameId*10000 +lprecognizeAlgo->mFrameDoneNum, 1, "lprecognizeAlgo");
    lprecognizeAlgo->save_image(ocl_mem->frame.getMat(0).ptr(), lprecognizeAlgo->mInputWidth,
        lprecognizeAlgo->mInputHeight,3, 1, "lprecognizeAlgo");
    #endif

    // lp recognize callback function
    auto onLpRecognizeResult = [&objectData](CvdlAlgoData* algoData)
    {
        LpRecognizeAlgo *lprecognizeAlgo = static_cast<LpRecognizeAlgo *>(algoData->algoBase);
        // this ocl will not use, free it here
        gst_buffer_unref(objectData.oclBuf);

        lprecognizeAlgo->mInferCnt--;
        objectData.flags |=LP_RECOGNIZE_FLAG_DONE;

        // check and process algoData
        try_process_algo_data(algoData);
    };

    // ASync detect, directly return after pushing request.
    start = g_get_monotonic_time();
    ret = lprecognizeAlgo->mIeLoader.do_inference_async(algoData, algoData->mFrameId,objId,
                                                        ocl_mem->frame, onLpRecognizeResult);
    stop = g_get_monotonic_time();
    lprecognizeAlgo->mInferCost += (stop - start);
    lprecognizeAlgo->mInferCnt++;
    lprecognizeAlgo->mInferCntTotal++;

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
static void lp_recognize_algo_func(gpointer userData)
{
    LpRecognizeAlgo *lprecognizeAlgo = static_cast<LpRecognizeAlgo*> (userData);
    CvdlAlgoData *algoData = new CvdlAlgoData;
    GST_LOG("\nlp_recognize_algo_func - new an algoData = %p\n", algoData);

    if(!lprecognizeAlgo->mNext) {
        GST_LOG("The lp recognize algo's next algo is NULL");
    }

    if(!lprecognizeAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(lprecognizeAlgo);

    if(algoData->mGstBuffer==NULL) {
        GST_ERROR("%s() - get null buffer\n", __func__);
        return;
    }
    GST_LOG("%s() - lp recognize = %p, algoData->mFrameId = %ld\n", __func__,
            lprecognizeAlgo, algoData->mFrameId);
    GST_LOG("%s() - get one buffer, GstBuffer = %p, refcout = %d, queueSize = %d,"\
            "algoData = %p, algoBase = %p\n",
        __func__, algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT (algoData->mGstBuffer),
        lprecognizeAlgo->mInQueue.size(), algoData, algoData->algoBase);

    for(unsigned int i=0; i< algoData->mObjectVec.size(); i++) {
        algoData->mObjectVec[i].flags &=
              ~(LP_RECOGNIZE_FLAG_DONE|LP_RECOGNIZE_FLAG_VALID);
    }

    // get input data and process it here, put the result into algoData
    // NV12-->BGR_Plannar
    for(unsigned int i=0; i< algoData->mObjectVec.size(); i++)
        process_one_object(algoData, algoData->mObjectVec[i], i);
    lprecognizeAlgo->mFrameDoneNum++;

}

LpRecognizeAlgo::LpRecognizeAlgo() : CvdlAlgoBase(lp_recognize_algo_func, this, NULL)
{
    mInputWidth = LP_INPUT_W;
    mInputHeight = LP_INPUT_H;
    mIeInited = false;
    mInCaps = NULL;
}


LpRecognizeAlgo::~LpRecognizeAlgo()
{
    wait_work_done();
    if(mInCaps)
        gst_caps_unref(mInCaps);
    g_print("LpRecognizeAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

void LpRecognizeAlgo::set_data_caps(GstCaps *incaps)
{
    // load IE and cnn model
    std::string filenameXML;
    const gchar *env = g_getenv("CVDL_LP_RECOGNIZE_MODEL_FULL_PATH");
    if(env) {
        filenameXML = std::string(env);
    }else{
        filenameXML = std::string(CVDL_MODEL_DIR_DEFAULT"/license_plate_recognize/license-plate-recognition-barrier-0001.xml");
    }
    algo_dl_init(filenameXML.c_str());

    //get data size of ie input
    GstFlowReturn ret = GST_FLOW_OK;
    int w, h, c;
    ret = mIeLoader.get_input_size(&w, &h, &c);

    if(ret==GST_FLOW_OK) {
        g_print("LpRecognizeAlgo: parse out the input size whc= %dx%dx%d\n", w, h, c);
        mInputWidth = w;
        mInputHeight = h;
    }

    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    //int oclSize = mInputWidth * mInputHeight * 3;
    mOclCaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    gst_caps_set_simple (mOclCaps, "width", G_TYPE_INT, mInputWidth, "height",
      G_TYPE_INT, mInputHeight, NULL);

    mImageProcessor.ocl_init(incaps, mOclCaps, IMG_PROC_TYPE_OCL_CRC, CRC_FORMAT_BGR_PLANNAR);
    mImageProcessor.get_input_video_size(&mImageProcessorInVideoWidth,
                                         &mImageProcessorInVideoHeight);
    gst_caps_unref (mOclCaps);

}

GstFlowReturn LpRecognizeAlgo::algo_dl_init(const char* modeFileName)
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
    mIeLoader.set_mean_and_scale(0.0f,  1.0f);

    // Load different Model based on different device.
    std::string strModelXml(modeFileName);
    std::string tmpFn = strModelXml.substr(0, strModelXml.rfind("."));
    std::string strModelBin = tmpFn + ".bin";
    GST_LOG("ClassificationModel bin = %s", strModelBin.c_str());
    GST_LOG("ClassificationModel xml = %s", strModelXml.c_str());
    ret = mIeLoader.read_model(strModelXml, strModelBin, IE_MODEL_LP_RECOGNIZE);
    
    if(ret != GST_FLOW_OK){
        GST_ERROR("IELoder failed to read model!");
        return GST_FLOW_ERROR;
    }
    return ret;
}

void ctc_ref_fp16(float* probabilities, float* output_sequences, float * output_prob,
                  int T_, int N_, int C_, int in_stride, bool original_layout) {

    // Fill output_sequences with -1
    for (int i = 0; i < T_; i++)
    {
        output_sequences[i] = (-1.0);
    }
    int output_index = 0;

    // Caffe impl
    for(int n = 0; n < N_; ++n)
    {
        int prev_class_idx = -1;

        for (int t = 0; t < T_; ++t)
        {
            // get maximum probability and its index
            int max_class_idx = 0;
            float* probs;
            float max_prob;

            if (original_layout)
            {
                probs = probabilities + t*in_stride;
                max_prob = probs[0];
                ++probs;

                for (int c = 1; c < C_; ++c, ++probs)
                {
                    if (*probs > max_prob)
                    {
                        max_class_idx = c;
                        max_prob = *probs;
                    }
                }
            }
            else
            {
                probs = probabilities + t;
                max_prob = probs[0];
                probs += T_;

                for (int c = 1; c < C_; ++c, probs += in_stride)
                {
                    if (*probs > max_prob)
                    {
                        max_class_idx = c;
                        max_prob = *probs;
                    }
                }
            }

            //if (max_class_idx != blank_index_
            //        && !(merge_repeated_&& max_class_idx == prev_class_idx))
            if (max_class_idx < C_-1 && !(1 && max_class_idx == prev_class_idx))
            {
            	output_prob[output_index] = max_prob;
                output_sequences[output_index] = ((float)max_class_idx);
                output_index++;
            }

            prev_class_idx = max_class_idx;

        }
    }
}

GstFlowReturn LpRecognizeAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlob,
                                                               int precision, CvdlAlgoData *outData, int objId) {
    GstFlowReturn ret = GST_FLOW_OK;
    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float>>(resultBlob);
    //InferenceEngine::Layout lo = resultBlob->getTensorDesc().getLayout();
    InferenceEngine::SizeVector sv = resultBlob->getTensorDesc().getDims();

    float * out = resultBlobFp32->data();

    int W = sv[0];//88
    int H = sv[1];//1
    int C = sv[2];//71

    float seq[88];
    float seq_prob[88];
    //T,N,C = 88,1,71
    ctc_ref_fp16(out, seq, seq_prob, W, H, C,   C, true);

    std::string result;
    int class_idx = -1;
    float prob_sum = 0;

    for(int col = 0; col < LPR_COLS; col ++){
        int idx = (int)(seq[col]);
        float prob = seq_prob[col];

        if(idx >= LPR_ROWS - 1 || idx < 0){
            continue;
        }

        std::string info = NAME_MAPPING[idx];
        if( idx >= 10 && idx <= 43){
            info = std::string("<") + info + std::string(">");
        }
        result += info;
        prob_sum += prob;
        class_idx = idx;
    }

   g_print("lp_recognize: prob = %f, label = %s\n", prob_sum, result.c_str());
   #if 1
   std::size_t pos = result.find("<");      // position of "<" in str
   if(pos>0 && pos<16) {
         std::string substr1 = result.substr (pos);
         std::string substr2 = result.substr (0, pos);
         result = substr1 + substr2;
          g_print("(correct)lp_recognize: prob = %f, label = %s\n", prob_sum, result.c_str());
    }
   #endif
   ObjectData &objData = outData->mObjectVec[objId];
   if(prob_sum >= 90) {
            objData.prob = prob_sum/100.0;
            objData.label = result;
            objData.objectClass =  class_idx;
            objData.flags |= LP_RECOGNIZE_FLAG_VALID;
    } else {
        result.clear();
        ret = GST_FLOW_ERROR;
    }

    return ret;
}
