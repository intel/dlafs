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
#include <gst/gst.h>
#include <gst/video/video.h>
#include "algobase.h"

//using namespace HDDLStreamFilter;
using namespace std;
using namespace cv;

#define QUEUE_ELEMENT_MAX_NUM 100


static void
algo_enter_thread (GstTask * task, GThread * thread, gpointer user_data)
{
    GST_DEBUG("enter algo thread.");
}

static void
algo_leave_thread (GstTask * task, GThread * thread, gpointer user_data)
{
    GST_DEBUG("leave algo thread.");
}

CvdlAlgoBase::CvdlAlgoBase(GstTaskFunction func, gpointer user_data, GDestroyNotify notify)
{
    g_rec_mutex_init (&mMutex);

    mNext = NULL;
    mPrev = NULL;

    mInferCnt = 0;
    mInferCntTotal = 0;
    mFrameIndex = 0;
    mFrameDoneNum = 0;
    mImageProcCost = 1;
    mInferCost = 1;

    /* Create task for this algo */
    if(func) {
        mTask = gst_task_new (func, user_data, notify);
        gst_task_set_lock (mTask, &mMutex);
        gst_task_set_enter_callback (mTask, algo_enter_thread, NULL, NULL);
        gst_task_set_leave_callback (mTask, algo_leave_thread, NULL, NULL);
    } else {
        mTask = NULL;
   }

    /* create queue, data is pointer*/
    //mInQueue = gst_atomic_queue_new(QUEUE_ELEMENT_MAX_NUM*sizeof(void *));
}


CvdlAlgoBase::~CvdlAlgoBase()
{
    wait_work_done();
    if(mTask) {
        if((gst_task_get_state(mTask) == GST_TASK_STARTED) ||
          (gst_task_get_state(mTask) == GST_TASK_PAUSED)) {
                gst_task_set_state(mTask, GST_TASK_STOPPED);
                mInQueue.flush();
                gst_task_join(mTask);
        }
        gst_object_unref(mTask);
        mTask = NULL;
    }
    mInQueue.close();
    mNext = mPrev = NULL;
    //gst_object_unref(mPool);
}

void CvdlAlgoBase::algo_connect(CvdlAlgoBase *algoTo)
{
    this->mNext = algoTo;
    algoTo->mPrev = this;
}

void CvdlAlgoBase::start_algo_thread()
{
    if(mTask)
        gst_task_start(mTask);
}

void CvdlAlgoBase::stop_algo_thread()
{
    if(mTask)
        gst_task_set_state(mTask, GST_TASK_STOPPED);
    mInQueue.flush();
    wait_work_done();
    if(mTask)
        gst_task_join(mTask);
}

void CvdlAlgoBase::queue_buffer(GstBuffer *buffer)
{
    CvdlAlgoData algoData(buffer);
    algoData.mFrameId = mFrameIndex++;
    if(buffer)
        algoData.mPts = GST_BUFFER_TIMESTAMP (buffer);
    mInQueue.put(algoData);
    GST_LOG("InQueue size = %d\n", mInQueue.size());
}

void CvdlAlgoBase::queue_out_buffer(GstBuffer *buffer)
{
    CvdlAlgoData algoData(buffer);
    algoData.mFrameId = mFrameIndex++;
    if(buffer)
        algoData.mPts = GST_BUFFER_TIMESTAMP (buffer);
    mInQueue.put(algoData);
    GST_LOG("InQueue size = %d\n", mInQueue.size());
}
int CvdlAlgoBase::get_in_queue_size()
{
    return mInQueue.size();
}

int CvdlAlgoBase::get_out_queue_size()
{
    //we did't have mOutQueue currently.
    return 0;
}
void CvdlAlgoBase::save_buffer(unsigned char *buf, int w, int h, int p, int id, char *info)
{
    char filename[128];
    sprintf(filename, "/home/lijunjie/temp/temp/%s-%dx%dx%d-%d.rgb",info,w,h,p,id);

#if 1
    int size = w*h;
    char *rgb = (char *)g_new0(char, w*h*p);
    for(int i=0;i<size;i++) {
        for(int j=0; j<p; j++)
            rgb[3*i + j] = buf[size * j + i];
    }
    FILE *fp = fopen (filename, "wb");
    if (fp) {
         fwrite (rgb, 1, w*h*p, fp);
         fclose (fp);
    }
    g_free(rgb);
#else
    FILE *fp = fopen (filename, "wb");
    if (fp) {
         fwrite (buf, 1, w*h*p, fp);
         fclose (fp);
    }
#endif
}