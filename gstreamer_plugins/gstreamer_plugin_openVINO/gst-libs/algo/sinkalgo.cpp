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
    mName = std::string(ALGO_SINK_NAME);
}

SinkAlgo::~SinkAlgo()
{
    CvdlAlgoData *algoData = NULL;
    while(mInQueue.size()>0)  {
            mInQueue.get(algoData);
            if(algoData->mGstBuffer) {
                gst_buffer_unref(algoData->mGstBuffer);
            }
            algoData->mObjectVec.clear();
            delete algoData;
    }
}

// dequeue a buffer with cvdlMeta data
GstBuffer* SinkAlgo::dequeue_buffer()
{
    GstBuffer* buf = NULL;
    CvdlAlgoData *algoData = NULL;
    gpointer meta_data = NULL;

    while(true)
    {
        if(!mInQueue.get(algoData)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return NULL;
        }
        // Send an invalid buffer for quit this task
        if(algoData->mGstBuffer==NULL) {
            g_print("%s() - got EOS buffer!\n",__func__);
            delete algoData;
            return NULL;
        }
        if(algoData->mObjectVec.size()>0)
            break;
    }
    buf = algoData->mGstBuffer;
    GST_LOG("cvdlfilter-dequeue: buf = %p(%d)\n", algoData->mGstBuffer,
        GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));

    // put object data as meta data
    VASurfaceID surface;
    VADisplay display;
    unsigned int color = 0x00FF00;
    surface= gst_get_mfx_surface (buf, NULL, &display);

    for(unsigned int i=0; i<algoData->mObjectVec.size(); i++) {
        VideoRect rect;
        std::vector<VideoPoint> &trajectoryPoints 
            = algoData->mObjectVec[i].trajectoryPoints;
        VideoPoint points[MAX_TRAJECTORY_POINTS_NUM];
        int count = trajectoryPoints.size();
        if(count>MAX_TRAJECTORY_POINTS_NUM)
            count = MAX_TRAJECTORY_POINTS_NUM;
        for(int i=0; i<count; i++)
            points[i] = trajectoryPoints[i];
        rect.x     = algoData->mObjectVec[i].rect.x;
        rect.y     = algoData->mObjectVec[i].rect.y;
        rect.width = algoData->mObjectVec[i].rect.width;
        rect.height= algoData->mObjectVec[i].rect.height;
        if(i==0)
            meta_data =
            cvdl_meta_create(display, surface, &rect, algoData->mObjectVec[i].label.c_str(), 
                             algoData->mObjectVec[i].prob, color, points, count);
        else
            cvdl_meta_add (meta_data, &rect, algoData->mObjectVec[i].label.c_str(),
                            algoData->mObjectVec[i].prob, color, points, count);
        //debug
        GST_LOG("%d - sinkalgo_output-%ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n", 
                mFrameDoneNum, algoData->mFrameId,i,algoData->mObjectVec[i].prob,
                algoData->mObjectVec[i].label.c_str(),
                rect.x, rect.y, rect.width, rect.height);
    }

    if(meta_data) {
        ((CvdlMeta *)meta_data)->meta_count = algoData->mObjectVec.size();
        gst_buffer_set_cvdl_meta(buf, (CvdlMeta *)meta_data);
    }
    delete algoData;
    return buf;
}


