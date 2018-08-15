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

#include "imageproc.h"
#include <common/common.h>
#include <ocl/oclpool.h>
using namespace HDDLStreamFilter;

ImageProcessor::ImageProcessor()
{
    mSrcFrame.reset (g_new0 (VideoFrame, 1), g_free);
    mDstFrame.reset (g_new0 (VideoFrame, 1), g_free);

    gst_video_info_init (&mInVideoInfo);
    gst_video_info_init (&mOutVideoInfo);

    mOclInited = false;
    mPool = NULL;
    mOclFormat = CRC_FORMAT_BGR_PLANNAR; // default is plannar
}

ImageProcessor::~ImageProcessor()
{
    if(mPool)
        gst_object_unref(mPool);

    gst_video_info_init (&mInVideoInfo);
    gst_video_info_init (&mOutVideoInfo);

    // relsease them by SharedPtr automatically
    //mSrcFrame = NULL;
    //mDstFrame = NULL;

    //TODO - destory OCL context
    // It will be done in OclVppBase::~OclVppBase ()
}
GstFlowReturn ImageProcessor::ocl_init(GstCaps *incaps, GstCaps *oclcaps, int vppType, int vppSubType)
{
    if (!incaps) {
        GST_ERROR ("Failed to init ImageProcessor: no in caps specified");
        return GST_FLOW_ERROR;
    }

    if (!gst_video_info_from_caps (&mInVideoInfo, incaps)) {
        GST_ERROR ("Failed to init ImageProcessor: no incaps info found!");
        return GST_FLOW_ERROR;
    }

    if(mPool) {
        GST_ERROR ("Double init ImageProcessor!");
        return GST_FLOW_ERROR;
    }

    mOclVppType = vppType;
    switch(mOclVppType){
        case IMG_PROC_TYPE_OCL_CRC:
            mOclFormat = (CRCFormat)vppSubType;
            if (!oclcaps) {
                GST_ERROR ("Failed to init ImageProcessor: no ocl caps specified");
                return GST_FLOW_ERROR;
            }
            if (!gst_video_info_from_caps (&mOutVideoInfo, oclcaps)) {
                GST_ERROR ("Failed to init ImageProcessor: no oclcaps info found!");
                return GST_FLOW_ERROR;
            }
            mPool = ocl_pool_create (oclcaps, mOutVideoInfo.size, 3, 16);
            break;
        case IMG_PROC_TYPE_OCL_BLENDER:
            mPool = NULL;
            break;
        default:
            break;
    }
    return GST_FLOW_OK;
}

void ImageProcessor::setup_ocl_context(VADisplay display)
{
    if(mOclInited)
        return;

    mContext = OclContext::create(display);
    if (!SHARED_PTR_IS_VALID (mContext)) {
        GST_DEBUG ("oclcrc: failed to create ocl ctx");
        return;
    }

    switch(mOclVppType){
        case IMG_PROC_TYPE_OCL_CRC:
             mOclVpp.reset (NEW_VPP_SHARED_PTR (OCL_VPP_CRC));
             // CRC has 3 kernel to output different format surface
             //if(mOclVppType==IMG_PROC_TYPE_OCL_CRC) {
             // Let OclVppCrc to choose the right kernel
             mOclVpp->setOclFormat(mOclFormat);
             //}
            break;
        case IMG_PROC_TYPE_OCL_BLENDER:
             mOclVpp.reset (NEW_VPP_SHARED_PTR (OCL_VPP_BLENDER));
             break;
        default:
            g_print("ocl: invalid vpp type!!!\n");
            break;
    }

    if (!SHARED_PTR_IS_VALID (mOclVpp) ||
        (OCL_SUCCESS != mOclVpp->setOclContext (mContext))) {
        GST_DEBUG ("oclcrc: failed to init ocl_vpp");
        mOclVpp.reset ();
        g_print("oclcrc: failed to init ocl_vpp");
    }

    mOclInited = true;
    mDisplay = display;
}


GstFlowReturn ImageProcessor::process_image_crc(GstBuffer* inbuf, GstBuffer** outbuf, VideoRect *crop)
{
    VADisplay display;

    /* Input data must be NV12 surface from mfxdec element */
    mSrcFrame->fourcc = video_format_to_va_fourcc (GST_VIDEO_INFO_FORMAT (&mInVideoInfo));
    mSrcFrame->surface= gst_get_mfx_surface (inbuf, &mInVideoInfo, &display);
    mSrcFrame->width  = mInVideoInfo.width;
    mSrcFrame->height = mInVideoInfo.height;

    if(mSrcFrame->surface == VA_INVALID_SURFACE) {
        GST_ERROR ("Failed to map VASurface to CL_MEM!");
        return GST_FLOW_ERROR;
    }
    setup_ocl_context(display);

    /* Output memory need to be allocated */
    GstBuffer *out_buf = NULL;
    OclMemory *out_mem = NULL;
    if (!(out_buf = ocl_buffer_alloc (mPool)))
            return GST_FLOW_ERROR;
    out_mem = ocl_memory_acquire (out_buf);
    if (!out_mem) {
        GST_ERROR ("Failed to acquire ocl memory from outbuf");
        return GST_FLOW_ERROR;
    }

    mDstFrame->fourcc = out_mem->fourcc;
    mDstFrame->mem    = out_mem->mem;
    mDstFrame->width  = mOutVideoInfo.width;
    mDstFrame->height = mOutVideoInfo.height;

    ocl_video_rect_set (&mSrcFrame->crop, crop);

    OclStatus status = mOclVpp->process (mSrcFrame, mDstFrame);

    if(status == OCL_SUCCESS) {
        *outbuf = out_buf;
        return GST_FLOW_OK;
    }

    *outbuf = NULL;
    gst_buffer_unref(out_buf);
    return GST_FLOW_ERROR;
}

/* blend cvdl osd onto orignal NV12 surface
 *    input: osd buffer
 *   *output: nv12 video buffer
 *    rect: the size of osd buffer
 */
GstFlowReturn ImageProcessor::process_image_blend(GstBuffer* inbuf, GstBuffer** outbuf, VideoRect *rect)
{
    VADisplay display;
    GstBuffer *osd_buf, *dst_buf;
    OclMemory *osd_mem;

    osd_buf = inbuf;
    dst_buf = *outbuf;

    // osd memory is OCL buffer
    osd_mem = ocl_memory_acquire (osd_buf);
    if (!osd_mem) {
        GST_ERROR ("Failed to acquire ocl memory from osd_buf");
        return GST_FLOW_ERROR;
    }
    mSrcFrame->fourcc = osd_mem->fourcc;
    mSrcFrame->mem    = osd_mem->mem;
    mSrcFrame->width  = rect->width;
    mSrcFrame->height = rect->height;

    /* Output data must be NV12 surface from mfxdec element */
    mDstFrame->fourcc = video_format_to_va_fourcc (GST_VIDEO_INFO_FORMAT (&mInVideoInfo));
    mDstFrame->surface= gst_get_mfx_surface (dst_buf, &mInVideoInfo, &display);
    mDstFrame->width  = mInVideoInfo.width;
    mDstFrame->height = mInVideoInfo.height;

    if(mDstFrame->surface == VA_INVALID_SURFACE) {
        GST_ERROR ("Failed to get VASurface!");
        return GST_FLOW_ERROR;
    }
    setup_ocl_context(display);

    if (mDstFrame->fourcc != OCL_FOURCC_NV12) {
        GST_ERROR ("only support RGBA blending on NV12 video frame");
        return GST_FLOW_ERROR;
    }
    ocl_video_rect_set (&mSrcFrame->crop, rect);

    OclStatus status = mOclVpp->process (mSrcFrame, mDstFrame);

    if(status == OCL_SUCCESS)
        return GST_FLOW_OK;
    else {
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;

}

GstFlowReturn ImageProcessor::process_image(GstBuffer* inbuf, GstBuffer** outbuf, VideoRect *crop)
{
    GstFlowReturn ret = GST_FLOW_OK;
    switch(mOclVppType){
        case IMG_PROC_TYPE_OCL_CRC:
            ret = process_image_crc(inbuf, outbuf, crop);
            break;
        case IMG_PROC_TYPE_OCL_BLENDER:
            ret = process_image_blend(inbuf, outbuf, crop); 
            break;
        default:
            ret = GST_FLOW_ERROR;
            break;
    }

    return ret;
}

