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
#include <interface/videodefs.h>
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
    cv::Rect rect;  // rect of detection
    cv::Rect rectROI; // rect to be classified

    /*score to decide whether is should be do classification */
    float score;

    std::vector<VideoPoint> trajectoryPoints; /* the trajectory Points of this object*/
    //std::atomic<int> flags;
    int flags;
    // Object buffer in OCL, format = BGR_Plannar
    // It will be used for IE inputdata
    GstBuffer *oclBuf;
    float figure_score(int w, int h) {
        float score = 0.0;
        int dx, dy;
        int centX = rect.x + rect.width/2;
        int centY = rect.y + rect.height/2;
        dx = fabs(centX - w/2);
        dy = fabs(centY - h/2);
        if((centY < h/3) || (centY > 2*h/3) || (rect.y + rect.height > 5*h/6))
            score =0.0;
        else
            score = 1.0*(rect.width * rect.height) / (dx*dy + w*h/16);
        return score;
    }
};

class CvdlAlgoBase;
class CvdlAlgoData;
using AsyncCallback = std::function<void(CvdlAlgoData* algoData)>;

class CvdlAlgoData{
public:
    CvdlAlgoData(){};
    CvdlAlgoData(GstBuffer *buf) {
        if(buf){
            mGstBuffer = buf;//gst_buffer_ref(buf);
        }else{
            mGstBuffer = NULL;
        }
        mFrameId = 0;
        mPts = 0;
    }
    ~CvdlAlgoData() {
        // Dont unref it, which will cause unref when copy this data structure into Queue
        if(mGstBuffer){
            //gst_buffer_unref(mGstBuffer);
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
    void queue_out_buffer(GstBuffer *buffer);
    void start_algo_thread();
    void stop_algo_thread();

    void wait_work_done() {
         // wait IE infer thread finished
         while((mInferCnt>0) ||(mInQueue.size()>0))
            g_usleep(10000);
    }
    int get_in_queue_size();
    int get_out_queue_size();
    void save_buffer(unsigned char *buf, int w, int h, int p, int id, char *info);

    virtual void set_data_caps(GstCaps *incaps)
    {

    }
    virtual GstFlowReturn parse_inference_result(
        InferenceEngine::Blob::Ptr &resultBlobPtr,
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

    /* The image size into the actual algo processing */
    int mInputWidth;
    int mInputHeight;

    /* link algo to form a algo chain/pipeline */
    CvdlAlgoBase *mNext;
    CvdlAlgoBase *mPrev;

    // queue input buffer
    thread_queue<CvdlAlgoData> mInQueue;

    // only used by last algo in pipeline
    thread_queue<CvdlAlgoData> mOutQueue;

    /* pool for allocate buffer for inference result, CPU buffer */
    GstBufferPool *mResultPool;

    std::atomic<int> mInferCnt;
	std::atomic<guint64> mInferCntTotal;

    int mFrameIndex;
    int mFrameDoneNum;
    gint64 mImageProcCost; /* in microseconds */
    gint64 mInferCost; /* in microseconds */
};
#endif
