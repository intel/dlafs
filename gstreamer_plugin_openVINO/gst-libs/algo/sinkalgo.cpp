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

/*
  *   SinkAlgo is used to collect all algoitem branch output.
  *   It is the last algoitem of algopipeline
  */

#include "sinkalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace HDDLStreamFilter;

SinkAlgo::SinkAlgo():CvdlAlgoBase(NULL, CVDL_TYPE_NONE)
{

}

SinkAlgo::~SinkAlgo()
{
    CvdlAlgoData algoData;
    while(mInQueue.size()>0)  {
            mInQueue.get(algoData);
            if(algoData.mGstBuffer) {
                gst_buffer_unref(algoData.mGstBuffer);
            }
            algoData.mObjectVec.clear();
    }
}

// dequeue a buffer with cvdlMeta data
GstBuffer* SinkAlgo::dequeue_buffer()
{
    GstBuffer* buf = NULL;
    CvdlAlgoData algoData;
    gpointer meta_data;

    while(true)
    {
        if(!mInQueue.get(algoData)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return NULL;
        }
        // Send an invalid buffer for quit this task
        if(algoData.mGstBuffer==NULL) {
            g_print("%s() - got EOS buffer!\n",__func__);
            return NULL;
        }
        if(algoData.mObjectVec.size()>0)
            break;
    }
    buf = algoData.mGstBuffer;
    GST_LOG("cvdlfilter-dequeue: buf = %p(%d)\n", algoData.mGstBuffer,
        GST_MINI_OBJECT_REFCOUNT(algoData.mGstBuffer));

    // put object data as meta data
    VASurfaceID surface;
    VADisplay display;
    unsigned int color = 0x00FF00;
    surface= gst_get_mfx_surface (buf, NULL, &display);

    for(unsigned int i=0; i<algoData.mObjectVec.size(); i++) {
        VideoRect rect;
        std::vector<VideoPoint> &trajectoryPoints 
            = algoData.mObjectVec[i].trajectoryPoints;
        VideoPoint points[MAX_TRAJECTORY_POINTS_NUM];
        int count = trajectoryPoints.size();
        if(count>MAX_TRAJECTORY_POINTS_NUM)
            count = MAX_TRAJECTORY_POINTS_NUM;
        for(int i=0; i<count; i++)
            points[i] = trajectoryPoints[i];
        rect.x     = algoData.mObjectVec[i].rect.x;
        rect.y     = algoData.mObjectVec[i].rect.y;
        rect.width = algoData.mObjectVec[i].rect.width;
        rect.height= algoData.mObjectVec[i].rect.height;
        if(i==0)
            meta_data =
            cvdl_meta_create(display, surface, &rect, algoData.mObjectVec[i].label.c_str(), 
                             algoData.mObjectVec[i].prob, color, points, count);
        else
            cvdl_meta_add (meta_data, &rect, algoData.mObjectVec[i].label.c_str(),
                            algoData.mObjectVec[i].prob, color, points, count);
        //debug
        GST_LOG("%d - sinkalgo_output-%ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n", 
                mFrameDoneNum, algoData.mFrameId,i,algoData.mObjectVec[i].prob,
                algoData.mObjectVec[i].label.c_str(),
                rect.x, rect.y, rect.width, rect.height);
    }
    ((CvdlMeta *)meta_data)->meta_count = algoData.mObjectVec.size();

    gst_buffer_set_cvdl_meta(buf, (CvdlMeta *)meta_data);
    return buf;
}


