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

/*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
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
#include "private.h"

class ObjectData{
public:
    ObjectData() : id(-1), objectClass(-1), prob(0.0), 
                 flags(0), score(0.0), oclBuf(NULL), mAuxData(NULL),
                 mAuxDataLen(0){};
    ~ObjectData()
   {
        trajectoryPoints.clear();
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
    CvdlAlgoData(): mGstBuffer(NULL) ,mFrameId(0), mPts(0),mOutputIndex(0),  mAllObjectDone(true),
                                                mGstBufferOcl(NULL), algoBase(NULL), ie_start(0), ie_duration(0)
    {
        mObjectVec.clear();
        mObjectVecIn.clear();
    }
    CvdlAlgoData(GstBuffer *buf) : mGstBuffer(buf), mFrameId(0), mPts(0),mOutputIndex(0), mAllObjectDone(true),
                                                mGstBufferOcl(NULL), algoBase(NULL), ie_start(0), ie_duration(0)
    {
        if(buf){
            //gst_buffer_ref(buf);
        }
        mObjectVec.clear();
        mObjectVecIn.clear();
     }
    ~CvdlAlgoData() {
        if(mGstBuffer){
            //gst_buffer_unref(mGstBuffer);
        }
        mObjectVec.clear();
        mObjectVecIn.clear();
    }
    GstBuffer *mGstBuffer;
    guint64 mFrameId;
    guint64 mPts;

    // For multiple output algo, 0 is output[0], 1 is for output[1]
    // default is 0
    gint mOutputIndex; 

    // If all objects are done.
    gboolean mAllObjectDone;

    // It was the resize of the whole image
    GstBuffer *mGstBufferOcl;

    // It was the object image, part of the whole image
    // IE result has been parsed into it
    std::vector<ObjectData> mObjectVec;
    std::vector<ObjectData> mObjectVecIn;

    CvdlAlgoBase* algoBase;
    gint64 ie_start;
    gint64 ie_duration;
};


class CvdlAlgoBase
{
public:
    CvdlAlgoBase(PostCallback func, guint cvdl_type=CVDL_TYPE_DL);
    virtual ~CvdlAlgoBase();

    void algo_connect(CvdlAlgoBase *algoTo);
    void algo_connect_with_index(CvdlAlgoBase *algoTo, int index);
    void queue_buffer(GstBuffer *buffer, guint w, guint h);
    void queue_out_buffer(GstBuffer *buffer);
    void start_algo_thread();
    void stop_algo_thread();

    void clear_queue();
    void wait_work_done() {
         // wait IE infer thread finished
         int times = 100;
         while((mInferCnt>0) ||(mInQueue.size()>0)) {
            g_usleep(100000);//100ms
            clear_queue();
            if(times--<=0) 
           {
                int value = mInferCnt;
                g_print("warning: mInferCnt = %d in %s\n", value, mName.c_str() );
                break;
            }
         }
    }
    int get_in_queue_size();
    int get_out_queue_size();

    GstFlowReturn init_ieloader(const char* modeFileName, guint ieType, std::string network_config=std::string("none"));
    void  init_dl_caps(GstCaps* incaps);
    virtual void set_data_caps(GstCaps *incaps);
    // only for dl algo
    virtual GstFlowReturn algo_dl_init(const char* modeFileName)
    {
        return GST_FLOW_OK;
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
    void save_buffer(unsigned char *buf, int w, int h, int p, int id, int bPlannar,const char *info);
    void save_image(unsigned char *buf, int w, int h, int p, int bPlannar, char *info);
    void print_objects(std::vector<ObjectData> &objectVec);

private:
        //It's not expected that class instances are copied, the operator= should be declared as private.
        //In this case, if an attempt to copy is made, the compiler produces an error.
        CvdlAlgoBase& operator=(const CvdlAlgoBase& src){return *this;}

public:
    gboolean mCapsInited;

    // which algo it belongs
    guint  mAlgoType;
    std::string mName;

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
    CvdlAlgoBase *mNext[MAX_DOWN_STREAM_ALGO_NUM];
    CvdlAlgoBase *mPrev;

    // queue input buffer
    thread_queue<CvdlAlgoData *> mInQueue;

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
