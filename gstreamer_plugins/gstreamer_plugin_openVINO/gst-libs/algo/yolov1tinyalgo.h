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

private:
    //It's not expected that class instances are copied, the operator= should be declared as private.
    //In this case, if an attempt to copy is made, the compiler produces an error.
    Yolov1TinyInternalData& operator=(const Yolov1TinyInternalData& src){return *this;}
    Yolov1TinyInternalData(const Yolov1TinyInternalData& src){/* do not create copies */};

    const int cGrideSize = DETECTION_GRIDE_SIZE;
    const int cClassNum  = DETECTION_CLASS_NUM;
    const int cBoxNumEachBlock = DETECTION_BOX_NUM_FOR_EACH_BLOCK;
public:
    RectF *mBoxes;
    float **mProbData;
    RectSortable *mRectSorted;
};

#endif
