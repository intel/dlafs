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
#include <mutex>

using namespace HDDLStreamFilter;

using namespace std;
// vpp lock for ocl context
static std::mutex vpp_mutext;

ImageProcessor::ImageProcessor()
{
    mSrcFrame.reset (g_new0 (VideoFrame, 1), g_free);
    mSrcFrame2.reset (g_new0 (VideoFrame, 1), g_free);
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

    // mSrcFrame/mDstFrame will be relseased by SharedPtr automatically
    // OCL context will be done in OclVppBase::~OclVppBase ()
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
        GST_ERROR("oclcrc: failed to create ocl ctx");
        return;
    }

    switch(mOclVppType){
        case IMG_PROC_TYPE_OCL_CRC:
             mOclVpp.reset (NEW_VPP_SHARED_PTR (OCL_VPP_CRC));
             // CRC has 3 kernel to output different format surface
             mOclVpp->setOclFormat(mOclFormat);
            break;
        case IMG_PROC_TYPE_OCL_BLENDER:
             mOclVpp.reset (NEW_VPP_SHARED_PTR (OCL_VPP_BLENDER));
             break;
        default:
            GST_ERROR("ocl: invalid vpp type!!!\n");
            break;
    }

    if (!SHARED_PTR_IS_VALID (mOclVpp) ||
        (OCL_SUCCESS != mOclVpp->setOclContext (mContext))) {
            GST_ERROR ("oclcrc: failed to init ocl_vpp");
            mOclVpp.reset ();
    }

    GST_LOG("oclcrc: success to init ocl_vpp\n");
    mOclInited = true;
    mDisplay = display;
}


void ImageProcessor::ocl_lock()
{
    vpp_mutext.lock();
}
void ImageProcessor::ocl_unlock()
{
    vpp_mutext.unlock();
}

GstFlowReturn ImageProcessor::process_image_crc(GstBuffer* inbuf,
    GstBuffer** outbuf, VideoRect *crop)
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

    // need lock it ?
    ocl_lock();
    OclStatus status = mOclVpp->process (mSrcFrame, mDstFrame);
    ocl_unlock();

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
GstFlowReturn ImageProcessor::process_image_blend(GstBuffer* inbuf,
        GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *rect)
{
    VADisplay display;
    GstBuffer *osd_buf, *dst_buf;
    OclMemory *osd_mem, *dst_mem;

    osd_buf = inbuf;
    dst_buf = *outbuf;

    dst_mem = ocl_memory_acquire (dst_buf);
    if (!dst_mem) {
        GST_ERROR ("Failed to acquire ocl memory from dst_buf");
        return GST_FLOW_ERROR;
    }

    // osd memory is OCL buffer
    osd_mem = ocl_memory_acquire (osd_buf);
    if (!osd_mem) {
        GST_ERROR ("Failed to acquire ocl memory from osd_buf");
        return GST_FLOW_ERROR;
    }
    //osd is RGBA
    mSrcFrame->fourcc = osd_mem->fourcc;
    mSrcFrame->mem    = osd_mem->mem;
    mSrcFrame->width  = rect->width;
    mSrcFrame->height = rect->height;

    /* input data must be NV12 surface from mfxdec element */
    // NV12 input
    mSrcFrame2->fourcc = video_format_to_va_fourcc (GST_VIDEO_INFO_FORMAT (&mInVideoInfo));
    mSrcFrame2->surface= gst_get_mfx_surface (inbuf2, &mInVideoInfo, &display);
    mSrcFrame2->width  = mInVideoInfo.width;
    mSrcFrame2->height = mInVideoInfo.height;

    // output is RGBA format
    mDstFrame->fourcc = dst_mem->fourcc;
    mDstFrame->mem    = dst_mem->mem;
    mDstFrame->width  = rect->width;
    mDstFrame->height = rect->height;

    if(mSrcFrame2->surface == VA_INVALID_SURFACE) {
        GST_ERROR ("Failed to get VASurface!");
        return GST_FLOW_ERROR;
    }
    setup_ocl_context(display);

    if (mSrcFrame2->fourcc != OCL_FOURCC_NV12) {
        GST_ERROR ("only support RGBA blending on NV12 video frame");
        return GST_FLOW_ERROR;
    }
    ocl_video_rect_set (&mSrcFrame->crop, rect);

    ocl_lock();
    OclStatus status = mOclVpp->process (mSrcFrame, mSrcFrame2, mDstFrame);
    ocl_unlock();

    if(status == OCL_SUCCESS)
        return GST_FLOW_OK;
    else {
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;

}

GstFlowReturn ImageProcessor::process_image(GstBuffer* inbuf,
    GstBuffer* inbuf2, GstBuffer** outbuf, VideoRect *crop)
{
    GstFlowReturn ret = GST_FLOW_OK;
    switch(mOclVppType){
        case IMG_PROC_TYPE_OCL_CRC:
            ret = process_image_crc(inbuf, outbuf, crop);
            break;
        case IMG_PROC_TYPE_OCL_BLENDER:
            ret = process_image_blend(inbuf, inbuf2, outbuf, crop); 
            break;
        default:
            ret = GST_FLOW_ERROR;
            break;
    }

    return ret;
}

