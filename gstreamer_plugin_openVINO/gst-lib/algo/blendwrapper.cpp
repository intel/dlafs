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

#include "blendwrapper.h"
#include <ocl/oclpool.h>
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

#include <string>
#include "imageproc.h"


#ifdef __cplusplus
extern "C" {
#endif

BlendHandle blender_create()
{
    CvdlBlender *cvdl_blender = (CvdlBlender *)g_new0(CvdlBlender,1);
    ImageProcessor *img_processor = new ImageProcessor;
    cvdl_blender->mImgProcessor = static_cast<void *>(img_processor);
    cvdl_blender->mInited = false;

    return (BlendHandle)cvdl_blender;
}

void blender_init(BlendHandle handle, GstPad* pad)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstVideoInfo info;
    int width, height, size;
    GstCaps *ocl_caps, *caps;

    if(cvdl_blender->mInited)
        return;

    // Get pad caps for video width/height
    caps = gst_pad_get_current_caps(pad);
    gst_video_info_from_caps (&info, caps);
    width = info.width;
    height = info.height;

    // ocl pool will allocate the same size osd buffer with BRG format
    size = width * height * 4;
    ocl_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGRx", NULL);
    gst_caps_set_simple (ocl_caps, "width", G_TYPE_INT, width, "height",
                         G_TYPE_INT, height, NULL);
    cvdl_blender->mOsdPool = ocl_pool_create (ocl_caps, size, 6,20);
    gst_caps_unref (ocl_caps);

    // init imgage processor: it will not allocate ocl buffer in it
    ImageProcessor *img_processor = static_cast<ImageProcessor *>(cvdl_blender->mImgProcessor);
    img_processor->ocl_init(caps, NULL, IMG_PROC_TYPE_OCL_BLENDER, -1);
    img_processor->get_input_video_size(&cvdl_blender->mImageWidth,
                                                      &cvdl_blender->mImageHeight);

    cvdl_blender->mInited = true;
}

void blender_destroy(BlendHandle handle)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    ImageProcessor *img_processor = static_cast<ImageProcessor *>(cvdl_blender->mImgProcessor);

    delete img_processor;
    if(cvdl_blender->mOsdPool)
        gst_object_unref(cvdl_blender->mOsdPool);
    cvdl_blender->mOsdPool = NULL;
    cvdl_blender->mImgProcessor = NULL;
    free(cvdl_blender);
}


static GstBuffer *get_free_buf(BlendHandle handle)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstBuffer *free_buf = NULL;
    OclMemory *free_mem = NULL;

    free_buf = ocl_buffer_alloc(cvdl_blender->mOsdPool);
    g_return_val_if_fail(free_buf, NULL);

    free_mem = ocl_memory_acquire(free_buf);
    if(!free_mem) {
        gst_buffer_unref(free_buf);
        return NULL;
    }
    return free_buf;
}

static GstBuffer *generate_osd(BlendHandle handle, GstBuffer *input_buf)
{
    //CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstBuffer *osd_buf = NULL;
    OclMemory *osd_mem = NULL;
    CvdlMeta *cvdl_meta = NULL;

    osd_buf = get_free_buf(handle);
    g_return_val_if_fail(osd_buf, NULL);

    cvdl_meta = gst_buffer_get_cvdl_meta(input_buf);
    if(!cvdl_meta){
        gst_buffer_unref(osd_buf);
        return NULL;
    }

    osd_mem = ocl_memory_acquire(osd_buf);

    InferenceMeta *inference_result = cvdl_meta->inference_result;
    int meta_count = cvdl_meta->meta_count;
    int i;
    cv::Mat mdraw = osd_mem->frame.getMat(0);

    for(i=0;i<meta_count && inference_result;i++){

        VideoRect *rect = &inference_result->rect;
        std::string strTxt;
        // Create an output string stream
        std::ostringstream stream_prob;
        stream_prob << std::fixed << std::setprecision(3) << inference_result->probility;

        // Write label and probility
        strTxt =std::string(inference_result->label) + "(prob= " + stream_prob.str() + ")";
        cv::putText(mdraw, strTxt, cv::Point(rect->x, rect->y), 1, 1.5, cv::Scalar(0, 255, 255), 2);

        // Draw rectangle on target object
        cv::Rect target_rect(rect->x, rect->y, rect->width, rect->height);
        cv::rectangle(mdraw, target_rect, cv::Scalar(0, 255, 0), 2);

        inference_result = inference_result->next;
    }
    if(i==meta_count)
        return osd_buf;

    return NULL;
}

GstBuffer* blender_process_cvdl_buffer(BlendHandle handle, GstBuffer* buffer)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstBuffer *osd_buf = NULL, *out_buf = NULL;
    VideoRect rect = {0, 0 , (unsigned int)cvdl_blender->mImageWidth,
                             (unsigned int)cvdl_blender->mImageHeight};
    //GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *dst_buf = get_free_buf(handle);
    g_return_val_if_fail(dst_buf, NULL);

    osd_buf = generate_osd(handle, buffer);
    if(!osd_buf) {
        GST_ERROR("Failed to generate osd surface for cvdl buffer!");
        return NULL;
    }
    out_buf = dst_buf;
    ImageProcessor *img_processor = static_cast<ImageProcessor *>(cvdl_blender->mImgProcessor);
    img_processor->process_image(osd_buf, buffer, &out_buf, &rect);

    // TODO: Set cvdl_meta to this buffer

    //release osd
    gst_buffer_unref(osd_buf);
 
    // TODO: Where to release Nv12 buffer and dst buffer?



    // TODO: how to make out_buf can be accepted by jpeg enc - RGBA or NV12?

    return dst_buf;
}


#ifdef __cplusplus
};
#endif

