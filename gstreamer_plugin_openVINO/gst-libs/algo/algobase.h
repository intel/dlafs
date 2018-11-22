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
#include "imageproc.h"
#include <ocl/oclmemory.h>
#include "ieloader.h"

const static guint CVDL_OBJECT_FLAG_DONE =0x1;

const static guint  CVDL_TYPE_DL = 0;
const static guint  CVDL_TYPE_CV = 1;
const static guint  CVDL_TYPE_NONE = 2;

class ObjectData{
public:
    ObjectData() : id(-1), objectClass(-1), prob(0.0), 
                 flags(0), score(0.0), oclBuf(NULL), mAuxData(NULL),
                 mAuxDataLen(0){};
    ~ObjectData()
   {
        trajectoryPoints.clear();
        //if(mAuxData)
        //    g_free(mAuxData);
        //mAuxData=NULL;
    };
    int id;
    int objectClass;
    float prob;
    std::string label;
    // It is based on the orignal video frame
    cv::Rect rect;  // rect of detection
    cv::Rect rectROI; // rect to be classified

    // flags the status of this object
    int flags;

    /*score to decide whether is should be do classification */
    float score;

    std::vector<VideoPoint> trajectoryPoints; /* the trajectory Points of this object*/

    // Buffer for ROI
    // Object buffer in OCL, format = BGR_Plannar
    // It will be used for IE inputdata
    GstBuffer *oclBuf;

    // Notice: need deep copy if push algoData to queue or free this buffer
    // This buffer should only used in the same algo item, not pass to next algo
    // or Don't malloc buffer for it, but only refer buffer to it
    void *mAuxData;
    gint mAuxDataLen;

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
using PostCallback = std::function<void(CvdlAlgoData* algoData)>;

class CvdlAlgoData{
public:
    CvdlAlgoData(): mGstBuffer(NULL) ,mFrameId(0), mPts(0), mAllObjectDone(true),
                                                mGstBufferOcl(NULL), algoBase(NULL){};
    CvdlAlgoData(GstBuffer *buf) : mGstBuffer(buf), mFrameId(0), mPts(0),mAllObjectDone(true),
                                                mGstBufferOcl(NULL), algoBase(NULL)
    {
        if(buf){
            //gst_buffer_ref(buf);
        }
     }
    ~CvdlAlgoData() {
        // Dont unref it, which will cause unref when copy this data structure into Queue
        if(mGstBuffer){
            //gst_buffer_unref(mGstBuffer);
        }
        mObjectVec.clear();
        mObjectVecIn.clear();
    }
    GstBuffer *mGstBuffer;
    guint64 mFrameId;
    guint64 mPts;

    // If all objects are done.
    gboolean mAllObjectDone;

    // It was the resize of the whole image
    GstBuffer *mGstBufferOcl;

    // It was the object image, part of the whole image
    // IE result has been parsed into it
    std::vector<ObjectData> mObjectVec;
    std::vector<ObjectData> mObjectVecIn;

    CvdlAlgoBase* algoBase;
};


class CvdlAlgoBase
{
public:
    CvdlAlgoBase(PostCallback func, guint cvdl_type=CVDL_TYPE_DL);
    virtual ~CvdlAlgoBase();

    void algo_connect(CvdlAlgoBase *algoTo);
    void queue_buffer(GstBuffer *buffer, guint w, guint h);
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

    GstFlowReturn init_ieloader(const char* modeFileName, guint ieType);
    void  init_dl_caps(GstCaps* incaps);
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
    void save_buffer(unsigned char *buf, int w, int h, int p, int id, int bPlannar,char *info);
    void save_image(unsigned char *buf, int w, int h, int p, int bPlannar, char *info);
    void print_objects(std::vector<ObjectData> &objectVec);
    
    gboolean mCapsInited;

    // which algo it belongs
    guint  mAlgoType;

   // There are 2 tasks,  DL task and CV task
   //  DL task - run with IE inference engine
   //  CV task - run without IE inference enngine
    guint mCvdlType;// DL or CV task

    /* Main task/thread to do the algo processing */
    GstTask *mTask;
    GRecMutex mMutex;

    IELoader mIeLoader;
    gboolean mIeInited;
    ImageProcessor mImageProcessor;

    /* The image size into the actual algo processing */
    int mInputWidth;
    int mInputHeight;

   // Orignal frame size
    int mImageProcessorInVideoWidth;
    int mImageProcessorInVideoHeight;

     /* Caps for orignal input video*/
    GstCaps *mInCaps;
     /* Caps for output surface of OCL, which has been CRCed, and as the input of detection algo */
    GstCaps *mOclCaps; 

    /* link algo to form a algo chain/pipeline */
    CvdlAlgoBase *mNext;
    CvdlAlgoBase *mPrev;

    // queue input buffer
    thread_queue<CvdlAlgoData> mInQueue;

    // Obsoleted unused buffers, which need deleted but delay some times
    CvdlAlgoData  *mObsoletedAlgoData;;

    /* pool for allocate buffer for inference result, CPU buffer */
    GstBufferPool *mResultPool;

   // DL task - algoData has been figured out, post process objects in algoData if need
   // CV task - use preprocessed picture to do specified CV algo processing.
    PostCallback postCb;

    std::atomic<int> mInferCnt;
    std::atomic<guint64> mInferCntTotal;

    // mutex for multiple objects sync
    std::mutex mAlgoDataMutex;

    int mFrameIndex;
    int mFrameDoneNum;
    gint64 mImageProcCost; /* in microseconds */
    gint64 mInferCost; /* in microseconds */

    int mFrameIndexLast;
    // It was used to generate object id
    gint mObjIndex;

    // debug
    FILE *fpOclResult;
};
#endif
