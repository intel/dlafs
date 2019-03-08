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


#ifndef __IMAGE_PROCESSOR_H__
#define __IMAGE_PROCESSOR_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gstbuffer.h>
#include <gst/gstpad.h>

#include <interface/videodefs.h>
#include <interface/vppinterface.h>

using namespace HDDLStreamFilter;

enum {
    IMG_PROC_TYPE_OCL_CRC = 0,     /* oclCrc     */
    IMG_PROC_TYPE_OCL_BLENDER = 1, /* oclBlender */
};

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    //parse input caps and create ocl buffer pool based on ocl caps
    //
    //  vppType: CRC or Blender
    //  vppSubType: only for CRC  - BRG, BGR_PLANNER, GRAY
    //
    GstFlowReturn ocl_init(GstCaps *incaps, GstCaps *oclcaps, int vppType, int vppSubType);
    //Process image: CRC
    //   Note: oclcontext will be setup when first call this function
    //
    GstFlowReturn process_image(GstBuffer* inbuf, GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *crop);
    //
    //  Interface API: set ocl format for output surface
    //      This format will be passed into OclVppCrc to load the right kernel
    //
    //void set_ocl_kernel_name(int oclFormat) {mOclFormat = oclFormat;};

    void get_input_video_size(int *w, int *h)
    {
        *w = mInVideoInfo.width;
        *h = mInVideoInfo.height;
    }

    void ocl_lock();
    void ocl_unlock();

    GstBufferPool*   mPool;   /* create buffer in src_pool */
    GstVideoInfo     mInVideoInfo;
    GstVideoInfo     mOutVideoInfo;

    VideoRect mCrop;
    SharedPtr<VideoFrame> mSrcFrame;  // 
    SharedPtr<VideoFrame> mSrcFrame2; // if blender - NV12
    SharedPtr<VideoFrame> mDstFrame;

    SharedPtr<VppInterface> mOclVpp;
    int mOclVppType;
private:
    void setup_ocl_context(VideoDisplayID display);
    GstFlowReturn process_image_crc(GstBuffer* inbuf, GstBuffer** outbuf, VideoRect *crop);
    GstFlowReturn process_image_blend(GstBuffer* inbuf, GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *rect);

    VideoDisplayID mDisplay;
    SharedPtr<OclContext> mContext;
    gboolean   mOclInited;
    CRCFormat   mOclFormat;
};
#endif
