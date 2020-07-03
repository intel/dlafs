/*
 *Copyright (C) 2018 Intel Corporation
 *
 *SPDX-License-Identifier: LGPL-2.1-only
 *
 *This library is free software; you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation;
 * version 2.1.
 *
 *This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string>
#include "yolov1tinyalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>
#include "algoregister.h"

using namespace HDDLStreamFilter;
using namespace std;

#if 0
const char* voc_names[] = { "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow",
                            "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa",
                            "train", "tvmonitor" };
#endif
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

Yolov1TinyInternalData::Yolov1TinyInternalData()
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

Yolov1TinyInternalData::~Yolov1TinyInternalData()
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

static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
}

Yolov1TinyAlgo::Yolov1TinyAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL),
mCurPts(0)
{
    mName = std::string(ALGO_YOLOV1_TINY_NAME);
    set_default_label_name();

    mInputWidth = DETECTION_INPUT_W;
    mInputHeight = DETECTION_INPUT_H;
}

Yolov1TinyAlgo::~Yolov1TinyAlgo()
{
    g_print("Yolov1TinyAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

GstFlowReturn Yolov1TinyAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

     mIeLoader.set_precision(InferenceEngine::Precision::U8, InferenceEngine::Precision::FP32);
     ret = init_ieloader(modeFileName, IE_MODEL_DETECTION);
    return ret;
}

GstFlowReturn Yolov1TinyAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("Yolov1TinyAlgo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    //Improvement need: put in into class member, not need new each time
    Yolov1TinyInternalData *internalData = new Yolov1TinyInternalData;
    if(!internalData) {
        GST_ERROR("Yolov1TinyInternalData is NULL!");
        return GST_FLOW_ERROR;
    }

    get_detection_boxes(input, internalData, 1, 1, 0);

    //Different label, do not merge.
    nms_sort(internalData);

    get_result(internalData, outData);

    delete internalData;

    return GST_FLOW_OK;
}

/**************************************************************************
  *
  *    private method
  *
  **************************************************************************/
    void Yolov1TinyAlgo::set_default_label_name()
    {
        //if (cClassNum == 9) {
            set_label_names(barrier_names);
        //} else {
        //    set_label_names(voc_names);
        //}
    }
    
    void Yolov1TinyAlgo::set_label_names(const char** label_names)
    {
        mLabelNames = label_names;
    }


//
//    total = 7 * 7 * 2
//    classes = 9
//
void Yolov1TinyAlgo::nms_sort(Yolov1TinyInternalData *internalData)
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

void Yolov1TinyAlgo::get_detection_boxes(
            float *ieResult, Yolov1TinyInternalData *internalData,
            int32_t w, int32_t h, int32_t onlyObjectness)
{
    float top2Thr = cProbThreshold * 4 / 3;// Top2 score threshold

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
            int32_t box_index = cGrideSize * cGrideSize * (cClassNum + cBoxNumEachBlock)
                                + (idx * cBoxNumEachBlock + n) * 4;
            internalData->mBoxes[index].x = (predictions[box_index + 0] + col) / cGrideSize * w;
            internalData->mBoxes[index].y = (predictions[box_index + 1] + row) / cGrideSize * h;
            internalData->mBoxes[index].w = pow(predictions[box_index + 2], 2) * w;
            internalData->mBoxes[index].h = pow(predictions[box_index + 3], 2) * h;

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

//    num = 7 * 7 * 2
//    thresh = 0.2
//    classes = 9
//
void Yolov1TinyAlgo::get_result(Yolov1TinyInternalData *internalData, CvdlAlgoData *outData)
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
            // The classification model will use the car face to do inference, that means we should
            //  crop the front part of car's object to be ROI, which is done in OpticalflowTrackAlgo::get_roi_rect
           object.rectROI =cv::Rect( object.rect.x ,   object.rect.y + object.rect.height/2,
                                                         object.rect.width,   object.rect.height/2);
            objectNum++;
            outData->mObjectVec.push_back(object);
            GST_LOG("%ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n",outData->mFrameId, idx,
                object.prob, object.label.c_str(), left, top, right - left + 1, bot - top + 1);
        }
    }
}
