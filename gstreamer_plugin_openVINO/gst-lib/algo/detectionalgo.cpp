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
#include "detectionalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace HDDLStreamFilter;
using namespace std;

const char* voc_names[] = { "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow",
                            "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa",
                            "train", "tvmonitor" };
const char* barrier_names[] = { "minibus", "minitruck", "car", "mediumbus", "mpv", "suv", "largetruck", "largebus",
                            "other" };

static int nms_comparator(const void *pa, const void *pb)
{
    RectSortable a = *(RectSortable *) pa;
    RectSortable b = *(RectSortable *) pb;
    float diff = a.fProbs[a.nIdx][b.nCls] - b.fProbs[b.nIdx][b.nCls];
    if (diff < 0) {
        return 1;
    } else if (diff > 0) {
        return -1;
    }
    return 0;
}

DetectionInternalData::DetectionInternalData()
{
    int boxNumSum = cGrideSize * cGrideSize * cBoxNumEachBlock;
    mBoxes = (RectF*) calloc(boxNumSum, sizeof(RectF));

    int probNum = cGrideSize * cGrideSize * cBoxNumEachBlock;
    mProbData = (float**) calloc(probNum, sizeof(float *));
    CHECK(mProbData);
    for (int j = 0; j < probNum; ++j) {
        mProbData[j] = (float*) calloc(cClassNum, sizeof(float));
        CHECK(mProbData[j]);
    }

    int total = cGrideSize * cGrideSize * cBoxNumEachBlock;
    mRectSorted = (RectSortable*) calloc(total, sizeof(RectSortable));
}

DetectionInternalData::~DetectionInternalData()
{
    if(mBoxes)
        free(mBoxes);
    mBoxes = NULL;

    int probNum = cGrideSize * cGrideSize * cBoxNumEachBlock;
    if (mProbData) {
        for (int j = 0; j < probNum; j++) {
            free(mProbData[j]); 
        }
        free(mProbData);
    }
    mProbData = NULL;

    if(mRectSorted)
        free(mRectSorted);
    mRectSorted = NULL;
}

/*
 * This is the main function of cvdl task:
 *     1. NV12 --> BGR_Planar
 *     2. async inference
 *     3. parse inference result and put it into in_queue of next algo
 */
static void detection_algo_func(gpointer userData)
{
    DetectionAlgo *detectionAlgo = static_cast<DetectionAlgo*> (userData);
    CvdlAlgoData *algoData = new CvdlAlgoData;
    GstFlowReturn ret;

    if(!detectionAlgo->mNext) {
        GST_WARNING("The next algo is NULL, return");
        return;
    }

    if(!detectionAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(detectionAlgo);

    // get input data and process it here, put the result into algoData
    // NV12-->BGR_Plannar
    GstBuffer *ocl_buf = NULL;
    VideoRect crop = {0,0, (unsigned int)detectionAlgo->mImageProcessorInVideoWidth,
                           (unsigned int)detectionAlgo->mImageProcessorInVideoHeight};
    detectionAlgo->mImageProcessor.process_image(algoData->mGstBuffer, &ocl_buf, &crop);
    if(ocl_buf==NULL) {
        GST_WARNING("Failed to do image process!");
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

    // Detect callback function
    auto onDetectResult = [](CvdlAlgoData* &algoData)
    {
        DetectionAlgo *detectionAlgo = static_cast<DetectionAlgo *>(algoData->algoBase);
        gst_buffer_unref(algoData->mGstBufferOcl);
        if(algoData->mResultValid){
            detectionAlgo->mNext->mInQueue.put(*algoData);
        }else{
            delete algoData;
        }
        detectionAlgo->mInferCnt--;
    };

    // ASync detect, directly return after pushing request.
    ret = detectionAlgo->mIeLoader.do_inference_async(algoData, algoData->mFrameId, -1, ocl_mem->frame, onDetectResult);
    detectionAlgo->mInferCnt++;
    detectionAlgo->mInferCntTotal++;

    if (ret!=GST_FLOW_OK) {
        GST_ERROR("IE: detect FAIL");
    }

    /**
        * Save current images to 'detectPoolTrck' that will be used for tracking.
        * Detect result will be saved to '_detectRsltPool' in the callback function ''.
        */
    //global->detectPoolTrck.put(DetectObjRslt { img, bDetect });

}


DetectionAlgo::DetectionAlgo() : CvdlAlgoBase(detection_algo_func, this, NULL)
{
    set_default_label_name();

    //mImageProcessor.set_ocl_kernel_name(CRC_FORMAT_BGR_PLANNAR);
    mInputWidth = DETECTION_INPUT_W;
    mInputHeight = DETECTION_INPUT_H;
    mIeInited = false;
}

DetectionAlgo::~DetectionAlgo()
{

}

void DetectionAlgo::set_default_label_name()
{
    if (cClassNum == 9) {
        set_label_names(barrier_names);
    } else {
        set_label_names(voc_names);
    }
}

void DetectionAlgo::set_label_names(const char** label_names)
{
    mLabelNames = label_names;
}


void DetectionAlgo::set_data_caps(GstCaps *incaps)
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
    //gst_caps_unref (mOclCaps);

    // load IE and cnn model
    std::string filenameXML = std::string(MODEL_DIR"/vehicle_detect/yolov1-tiny.xml");
    algo_dl_init(filenameXML.c_str());
}

GstFlowReturn DetectionAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;
    if(mIeInited)
        return ret;
    mIeInited = true;

    ret = mIeLoader.set_device(InferenceEngine::TargetDevice::eHDDL);
    if(ret != GST_FLOW_OK){
        GST_ERROR("IE failed to set device be eHDDL!");
        return GST_FLOW_ERROR;
    }

    // Load different Model based on different device.
    std::string strModelXml(modeFileName);
    std::string tmpFn = strModelXml.substr(0, strModelXml.rfind("."));
    std::string strModelBin = tmpFn + ".bin";
    GST_DEBUG("DetectModel bin = %s", strModelBin.c_str());
    GST_DEBUG("DetectModel xml = %s", strModelXml.c_str());
    ret = mIeLoader.read_model(strModelXml, strModelBin, IE_MODEL_DETECTION);
    
    if(ret != GST_FLOW_OK){
        GST_ERROR("IELoder failed to read model!");
        return GST_FLOW_ERROR;
    }
    return ret;
}


/*
 *    total = 7 * 7 * 2 
 *    classes = 9
 */
void DetectionAlgo::nms_sort(DetectionInternalData *internalData)
{
    int total = cGrideSize * cGrideSize * cBoxNumEachBlock;
    int32_t idx, m, n;
    MathUtils utils;
    RectSortable *s = internalData->mRectSorted;
    if (NULL == s) {
        GST_ERROR("mRectSorted is NULL!");
        std::exit(EXIT_FAILURE);
    }

    // idx = 0 ~ 7*7*2-1
    for (idx = 0; idx < total; ++idx) {
        s[idx].nIdx = idx;
        s[idx].nCls = 0;
        s[idx].fProbs = internalData->mProbData;
    }

    // n = 0~8
    for (n = 0; n < cClassNum; ++n) {
        for (idx = 0; idx < total; ++idx) {
            s[idx].nCls = n;
        }
        qsort(s, total, sizeof(RectSortable), nms_comparator);

        for (idx = 0; idx < total; ++idx) {
            if (internalData->mProbData[s[idx].nIdx][n] == 0) {
                continue;
            }
            RectF a = internalData->mBoxes[s[idx].nIdx];
            for (m = idx + 1; m < total; ++m) {
                RectF b = internalData->mBoxes[s[m].nIdx];
                if (utils.box_iou(a, b) > cNMSThreshold) {
                    internalData->mProbData[s[m].nIdx][n] = 0;
                }
            }
        }
    }
}

void DetectionAlgo::get_detection_boxes(
            float *ieResult, DetectionInternalData *internalData,
            int32_t w, int32_t h, int32_t onlyObjectness)
{
    float top2Thr = cProbThreshold* 4 / 3;// Top2 score threshold

    int idx, m, n;
    float *predictions = ieResult;
    int32_t count = cGrideSize * cGrideSize; /* 7*7=49 */
    for (idx = 0; idx < count; idx++) {
        int row = idx / cGrideSize;
        int col = idx % cGrideSize;

        for (n = 0; n < cBoxNumEachBlock; ++n) {/* l.n == 2*/
            // index = 2 * idx + n
            // pIndex = 7*7*9 + 2*idx+n
            int index = idx * cBoxNumEachBlock + n;
            int pIndex = cGrideSize * cGrideSize * cClassNum + idx * cBoxNumEachBlock + n;
            float scale = predictions[pIndex];
            // box_index = 7 * 7 * (9 + 2) + 4 * (2 *index +n)
            int32_t box_index = cGrideSize * cGrideSize * (cClassNum + cBoxNumEachBlock) + (idx * cBoxNumEachBlock + n) * 4;
            internalData->mBoxes[index].x = (predictions[box_index + 0] + col) / cGrideSize * w;
            internalData->mBoxes[index].y = (predictions[box_index + 1] + row) / cGrideSize * h;
            internalData->mBoxes[index].w = pow(predictions[box_index + 2], 1) * w;
            internalData->mBoxes[index].h = pow(predictions[box_index + 3], 1) * h;

            float ascore[2] = { 0 };
            int32_t bHave = 0;
            int32_t maxscoreid = 0;

            for (m = 0; m < cClassNum; ++m) {/* l.nClss = 9 */
                int32_t class_index = idx * cClassNum;
                float prob = scale * predictions[class_index + m];
                internalData->mProbData[index][m] = prob > cProbThreshold ? prob : 0;

                // new modify
                {
                    if (internalData->mProbData[index][m] > cProbThreshold) {
                        bHave = 1;
                    }

                    // record top 2 score!
                    if (prob > ascore[0]) {
                        maxscoreid = m;
                        ascore[1] = ascore[0];
                        ascore[0] = prob;
                    } else if (prob > ascore[1]) {
                        ascore[1] = prob;
                    }
                }
            }
            if (onlyObjectness) {
                internalData->mProbData[index][0] = scale;
            }

            // new modify
            if (!bHave) {
                if (ascore[0] + ascore[1] > top2Thr) {
                    internalData->mProbData[index][maxscoreid] = ascore[0] + ascore[1];
                }
            }
        }
    }
}



/*    num = 7 * 7 * 2
 *    thresh = 0.2
 *    classes = 9
 */
void DetectionAlgo::get_result(DetectionInternalData *internalData, CvdlAlgoData *outData)
{
    int num = cGrideSize * cGrideSize * cBoxNumEachBlock;
    int idx;
    MathUtils utils;
    const char** labels = mLabelNames;
    int objectNum = 0;
    int orignalVideoWidth = mImageProcessorInVideoWidth;
    int orignalVideoHeight = mImageProcessorInVideoHeight;
    outData->mObjectVec.clear();

    for (idx = 0; idx < num; ++idx) {
        int32_t nclass = utils.get_max_index(internalData->mProbData[idx], cClassNum);
        float prob = internalData->mProbData[idx][nclass];

        if (prob > cProbThreshold) {
            ObjectData object;

            object.id = objectNum;
            object.objectClass = nclass;
            object.label = std::string(labels[nclass]);
            object.prob = prob;

            RectF b = internalData->mBoxes[idx];
            b.h = MAX(b.h, 0);
            b.w = MAX(b.w, 0);

            int left  = (b.x - b.w / 2.) * orignalVideoWidth;
            int right = (b.x + b.w / 2.) * orignalVideoWidth;
            int top   = (b.y - b.h / 2.) * orignalVideoHeight;
            int bot   = (b.y + b.h / 2.) * orignalVideoHeight;

            left = MAX(0, left);
            right = MAX(0, right);
            top = MAX(0, top);
            bot = MAX(0, bot);

            left = MIN(left, orignalVideoWidth - 1);
            right = MIN(right, orignalVideoWidth - 1);
            top = MIN(top, orignalVideoHeight - 1);
            bot = MIN(bot, orignalVideoHeight - 1);

            if (right - left <= 0) {
                continue;
            }
            if (bot - top <= 0) {
                continue;
            }
            object.rect = cv::Rect(left, top, right - left + 1, bot - top + 1);
            objectNum++;
            outData->mObjectVec.push_back(object);
        }
    }

    if(objectNum>=1){
        // put objectVec into output_queue
        outData->mResultValid = true;
    } else {
        outData->mResultValid = false;
    }
}

GstFlowReturn DetectionAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    DetectionInternalData *internalData = new DetectionInternalData;
    if(!internalData) {
        GST_ERROR("DetectionInternalData is NULL!");
        return GST_FLOW_ERROR;
    }

    get_detection_boxes(input, internalData, 1, 1, 0);

    //Different label, do not merge.
    nms_sort(internalData);

    get_result(internalData, outData);

    delete internalData;

    return GST_FLOW_OK;
}
    
