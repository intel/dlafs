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
#ifndef __CVDL_ALGO_BASE_H__
#define __CVDL_ALGO_BASE_H__

#include <gst/gstbuffer.h>
#include <gst/gstpad.h>
#include <gst/gstinfo.h>
#include "queue.h"
#include <opencv2/opencv.hpp>
#include <inference_engine.hpp>
#include <atomic>
#include <vector>

enum {
    CVDL_TYPE_DL = 0,
    CVDL_TYPE_CV = 1,
};

class ObjectData{
public:
    ObjectData() {flags = 0;};
    ~ObjectData() { trajectoryPoints.clear(); };
    int id;
    int objectClass;
    float prob;
    std::string label;
    // It is based on the orignal video frame
    cv::Rect rect;
    std::vector<cv::Point> trajectoryPoints; /* the trajectory Points of this object*/
    //std::atomic<int> flags;
    int flags;
    // Object buffer in OCL, format = BGR_Plannar
    // It will be used for IE inputdata
    GstBuffer *oclBuf;
};

class CvdlAlgoBase;
class CvdlAlgoData;
using AsyncCallback = std::function<void(CvdlAlgoData* &algoData)>;

class CvdlAlgoData{
public:
    CvdlAlgoData(){};
    CvdlAlgoData(GstBuffer *buf) {
        if(buf){
            mGstBuffer = gst_buffer_ref(buf);
        }else{
            mGstBuffer = NULL;
        }
    }
    ~CvdlAlgoData() {
        if(mGstBuffer){
            gst_buffer_unref(mGstBuffer);
        }
        mObjectVec.clear();

    }
    GstBuffer *mGstBuffer;
    guint64 mFrameId;
    guint64 mPts;

    // inference data is valid
    gboolean mResultValid;

    // It was the resize of the whole image
    GstBuffer *mGstBufferOcl;

    // It was the object image, part of the whole image
    // IE result has been parsed into it
    std::vector<ObjectData> mObjectVec;

    CvdlAlgoBase* algoBase;
};


class CvdlAlgoBase
{
public:
    CvdlAlgoBase(GstTaskFunction func, gpointer user_data, GDestroyNotify notify);
    virtual ~CvdlAlgoBase();

    void algo_connect(CvdlAlgoBase *algoTo);
    void queue_buffer(GstBuffer *buffer);
    void start_algo_thread();
    void stop_algo_thread();

    virtual void set_data_caps(GstCaps *incaps)
    {

    }
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                  int precision, CvdlAlgoData *outData, int objId)
    {
        return GST_FLOW_OK;
    }

    // dequeue a processed buffer with metaData, which is only used for the last algo in the algo pipeline
    virtual GstBuffer* dequeue_buffer()
    {
        return NULL;
    }

    guint  mAlgoType;
    guint  mCvdlType;

    /* Main task/thread to do the algo processing */
    GstTask *mTask;
    GRecMutex mMutex;

    /* orignal input video size */
    int mOrignalVideoWidth;
    int mOrignalVideoHeight;

    /* The image size into the actual algo processing */
    int mInputWidth;
    int mInputHeight;

    /* link algo to form a algo chain/pipeline */
    CvdlAlgoBase *mNext;
    CvdlAlgoBase *mPrev;

    /* pool to allocate buffer for cv/dl algorithm processing, OCL buffer 
       *    buffer size is: mInputWidth x mInputHeight
       */
    GstBufferPool *mPool;

#if 0
    /* queue for input buffer, the pointer to CvdlAlgoData*/
    GstAtomicQueue *mInQueue;
    /* queue for output buffer, which shoule be created in the next linked algorithm */
    GstAtomicQueue *mOutQueue;
#else
    thread_queue<CvdlAlgoData> mInQueue;
#endif
    /* pool for allocate buffer for inference result, CPU buffer */
    GstBufferPool *mResultPool;
    /* algo result data, which will*/
    //gpoint *result;

    std::atomic<int> mInferCnt;
	std::atomic<guint64> mInferCntTotal;
};
#endif
