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

#ifndef __IMAGE_PROCESSOR_H__
#define __IMAGE_PROCESSOR_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gstbuffer.h>
#include <gst/gstpad.h>
#include "va/va.h"

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

    /* parse input caps and create ocl buffer pool based on ocl caps
       *
       *  vppType: CRC or Blender
       *  vppSubType: only for CRC  - BRG, BGR_PLANNER, GRAY
       */
    GstFlowReturn ocl_init(GstCaps *incaps, GstCaps *oclcaps, int vppType, int vppSubType);
    /*Process image: CRC
       *   Note: oclcontext will be setup when first call this function
       */
    GstFlowReturn process_image(GstBuffer* inbuf, GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *crop);
    /*
       *  Interface API: set ocl format for output surface
       *      This format will be passed into OclVppCrc to load the right kernel
       */
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

    //SharedPtr<VppInterface> mOclCrc;
    SharedPtr<VppInterface> mOclVpp;
    int mOclVppType;
private:
    void setup_ocl_context(VADisplay display);
    GstFlowReturn process_image_crc(GstBuffer* inbuf, GstBuffer** outbuf, VideoRect *crop);
    GstFlowReturn process_image_blend(GstBuffer* inbuf, GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *rect);

    VADisplay mDisplay;
    SharedPtr<OclContext> mContext;
    gboolean   mOclInited;
    CRCFormat   mOclFormat;
};
#endif
