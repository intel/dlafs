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
#ifndef __ALGO_YOLOV1_TINY_H__
#define __ALGO_YOLOV1_TINY_H__

#include "algobase.h"
#include <gst/gstbuffer.h>
#include "mathutils.h"

#define DETECTION_GRIDE_SIZE  7
#define DETECTION_CLASS_NUM   9
#define DETECTION_BOX_NUM_FOR_EACH_BLOCK 2
#define DETECTION_PROB_THRESHOLD 0.200000003
//#define DETECTION_PROB_THRESHOLD 0.500000003
#define DETECTION_NMS_THRESHOLD 0.4

#define DETECTION_INPUT_W 448    // detect size
#define DETECTION_INPUT_H 448    // detect size

typedef struct {
    int32_t nIdx;
    int32_t nCls;
    float **fProbs;
} RectSortable;

typedef struct _Yolov1TinyResultData Yolov1TinyResultData;
struct _Yolov1TinyResultData{
    GstBuffer *buffer;
    guint64 pts;
    int object_num;
    std::vector<ObjectData> objects;
};

class Yolov1TinyInternalData;
class Yolov1TinyAlgo : public CvdlAlgoBase 
{
public:
    Yolov1TinyAlgo();
    virtual ~Yolov1TinyAlgo();
    //virtual void set_data_caps(GstCaps *incaps);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId);
private:
    const int cGrideSize = DETECTION_GRIDE_SIZE;/* 7 */
    const int cClassNum  = DETECTION_CLASS_NUM;/* 9 */
    const int cBoxNumEachBlock = DETECTION_BOX_NUM_FOR_EACH_BLOCK; /* 2 */
    const float cProbThreshold = DETECTION_PROB_THRESHOLD; /* 0.5 */
    const float cNMSThreshold = DETECTION_NMS_THRESHOLD; /* 0.4 */

    guint64 mCurPts;
    Yolov1TinyResultData mResultData;
    const char** mLabelNames;

    void set_default_label_name();
    void set_label_names(const char** label_names);

    //int nms_comparator(const void *pa, const void *pb);
    void nms_sort(Yolov1TinyInternalData *internalData);
    void get_detection_boxes(
            float *ieResult, Yolov1TinyInternalData *internalData,
            int32_t w, int32_t h, int32_t onlyObjectness);
    void get_result(Yolov1TinyInternalData *internalData, CvdlAlgoData *outData);
};


// Internal data for detection
// allocate one for each detection and free it when done
class Yolov1TinyInternalData{
public:
    Yolov1TinyInternalData();
    ~Yolov1TinyInternalData();

    const int cGrideSize = DETECTION_GRIDE_SIZE;
    const int cClassNum  = DETECTION_CLASS_NUM;
    const int cBoxNumEachBlock = DETECTION_BOX_NUM_FOR_EACH_BLOCK;

    RectF *mBoxes;
    float **mProbData;
    RectSortable *mRectSorted;
};

#endif
