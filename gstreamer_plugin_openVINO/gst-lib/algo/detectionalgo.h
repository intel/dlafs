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
#ifndef __DETECTION_ALGO_H__
#define __DETECTION_ALGO_H__

#include "algobase.h"
#include "ieloader.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include "mathutils.h"

#define DETECTION_GRIDE_SIZE  7
#define DETECTION_CLASS_NUM   9
#define DETECTION_BOX_NUM_FOR_EACH_BLOCK 2
//#define DETECTION_PROB_THRESHOLD 0.200000003
#define DETECTION_PROB_THRESHOLD 0.500000003
#define DETECTION_NMS_THRESHOLD 0.4

#define DETECTION_INPUT_W 448    // detect size
#define DETECTION_INPUT_H 448    // detect size


typedef struct {
	int32_t nIdx;
	int32_t nCls;
	float **fProbs;
} RectSortable;

typedef struct _DetectionResultData DetectionResultData;
struct _DetectionResultData{
    GstBuffer *buffer;
    guint64 pts;
    int object_num;
    std::vector<ObjectData> objects;
};

class DetectionInternalData;
class DetectionAlgo : public CvdlAlgoBase 
{
public:
    DetectionAlgo();
    virtual ~DetectionAlgo();
    virtual void set_data_caps(GstCaps *incaps);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId);
    void set_default_label_name();
    void set_label_names(const char** label_names);

    //int nms_comparator(const void *pa, const void *pb);
    void nms_sort(DetectionInternalData *internalData);
    void get_detection_boxes(
            float *ieResult, DetectionInternalData *internalData,
            int32_t w, int32_t h, int32_t onlyObjectness);
    void get_result(DetectionInternalData *internalData, CvdlAlgoData *outData);

    IELoader mIeLoader;
    gboolean mIeInited;
    ImageProcessor mImageProcessor;
    GstCaps *mInCaps;  /* Caps for orignal input video*/
    GstCaps *mOclCaps; /* Caps for output surface of OCL, which has been CRCed, and as the input of detection algo */

    int mImageProcessorInVideoWidth;
    int mImageProcessorInVideoHeight;

    const int cGrideSize = DETECTION_GRIDE_SIZE;
    const int cClassNum  = DETECTION_CLASS_NUM;
    const int cBoxNumEachBlock = DETECTION_BOX_NUM_FOR_EACH_BLOCK;
    const float cProbThreshold = DETECTION_PROB_THRESHOLD;
    const float cNMSThreshold = DETECTION_NMS_THRESHOLD;

    guint64 mCurPts;
    DetectionResultData mResultData;

    const char** mLabelNames;
};


// Internal data for detection
// allocate one for each detection and free it when done
class DetectionInternalData{
public:
    DetectionInternalData();
    ~DetectionInternalData();

    const int cGrideSize = DETECTION_GRIDE_SIZE;
    const int cClassNum  = DETECTION_CLASS_NUM;
    const int cBoxNumEachBlock = DETECTION_BOX_NUM_FOR_EACH_BLOCK;

    RectF *mBoxes;
    float **mProbData;
    RectSortable *mRectSorted;
};

#endif
