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

void blender_init(BlendHandle handle, GstCaps *caps)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstVideoInfo info;
    int width, height;//, size;
    GstCaps *ocl_caps;

    if(cvdl_blender->mInited)
        return;

    // Get pad caps for video width/height
    //caps = gst_pad_get_current_caps(pad);
    gst_video_info_from_caps (&info, caps);
    width = info.width;
    height = info.height;

    // ocl pool will allocate the same size osd buffer with BRGA format
    //size = width * height * 4;
    ocl_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGRA", NULL);
    gst_caps_set_simple (ocl_caps, "width", G_TYPE_INT, width, "height",
                         G_TYPE_INT, height, NULL);

    gst_video_info_from_caps (&info, ocl_caps);
    cvdl_blender->mOsdPool = ocl_pool_create (ocl_caps, info.size, 4,10);
    gst_caps_unref (ocl_caps);

    // init imgage processor: it will not allocate ocl buffer in it
    ImageProcessor *img_processor = static_cast<ImageProcessor *>(cvdl_blender->mImgProcessor);
    img_processor->ocl_init(caps, NULL, IMG_PROC_TYPE_OCL_BLENDER, -1);
    img_processor->get_input_video_size(&cvdl_blender->mImageWidth,
                                        &cvdl_blender->mImageHeight);

    cvdl_blender->mInited = true;
    //gst_caps_unref(caps);
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

    free_mem->purpose = 1;
    return free_buf;
}

#if 0
// test code
static void buffer_test(GstBuffer *buf)
{
    OclMemory *buf_mem = NULL;

    buf_mem = ocl_memory_acquire(buf);
    if(!buf_mem) {
        GST_ERROR("%s() - Failed to get ocl memory!\n",__func__);
        return;
    }

    cv::Mat mdraw = buf_mem->frame.getMat(0);
    cv::rectangle(mdraw, cv::Rect(0,0,100,100), cv::Scalar(0, 255, 0), 2);
    cv::rectangle(mdraw, cv::Rect(buf_mem->width-100,0,100,100), cv::Scalar(0, 255, 0), 2);
    cv::rectangle(mdraw, cv::Rect(0,buf_mem->height-100,100,100), cv::Scalar(0, 255, 0), 2);
    cv::rectangle(mdraw, cv::Rect(buf_mem->width-100,buf_mem->height-100,100,100), cv::Scalar(0, 255, 0), 2);
    cv::rectangle(mdraw, cv::Rect(buf_mem->width/2-50,buf_mem->height/2-50,100,100), cv::Scalar(0, 255, 0), 2);
    cv::rectangle(mdraw, cv::Rect(0,0,1600,1200), cv::Scalar(255, 255, 255), 2);

    //memset((void *)((unsigned char *)buf_mem->mem+600*1600*4), 0x7F, 1600*4);
}
#endif

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
    //g_return_val_if_fail(osd_mem, NULL);
    if(!osd_mem) {
        gst_buffer_unref(osd_buf);
        return NULL;
    }
    osd_mem->purpose = 2;

    std::ostringstream stream_ts;
    GstClockTime ts_ms = GST_BUFFER_PTS (input_buf)/1000000; //ms
    int hour,minute,second,ms;
    second = ts_ms/1000;
    hour = second/3600;
    minute = (second - hour*3600)/60;
    second = second % 60;
    ms = ts_ms % 1000;
    stream_ts << std::setfill('0') << std::setw(2) << hour << ":" << std::setfill('0') << std::setw(2) << minute << ":";
    stream_ts << std::setfill('0') << std::setw(2) << second << "." << std::setw(3) << ms;

    InferenceMeta *inference_result = cvdl_meta->inference_result;
    int meta_count = cvdl_meta->meta_count;
    int i;
#ifdef USE_OPENCV_3_4_x
    cv::Mat mdraw = osd_mem->frame.getMat(0);
#else
    cv::Mat mdraw = osd_mem->frame.getMat(cv::ACCESS_RW);
#endif
    uint32_t x,y;
    cv::rectangle(mdraw, cv::Rect(0,0,osd_mem->width, osd_mem->height), cv::Scalar(0, 0, 0, 0), cv::FILLED);
    cv::putText(mdraw, stream_ts.str(), cv::Point(10, 30), 1, 1.8, cv::Scalar(255, 0, 255, 255), 2);//RGBA

    for(i=0;i<meta_count && inference_result;i++){

        VideoRect *rect = &inference_result->rect;
        std::string strTxt;
        // Create an output string stream
        std::ostringstream stream_prob;
        stream_prob << std::fixed << std::setprecision(3) << inference_result->probility;

        x = rect->x;
        y = rect->y + 30;

        // check if a small object
        if( ((int)rect->width < osd_mem->width/10) ||
             ((int)rect->height < osd_mem->height/10)  ||
             (rect->width/(1.0+rect->height) > 3.0)  ||
             (rect->height/(1.0+rect->width) > 3.0)) {
             // Write label and probility
            strTxt = std::string(inference_result->label) + std::string("[") + stream_prob.str() + std::string("]") ;
             cv::putText(mdraw, strTxt, cv::Point( rect->x,  rect->y - 15), 1, 1.8, cv::Scalar(255 , 10, 255,255), 2);//RGBA
      } else {
             // Write label and probility
            strTxt = std::string(inference_result->label);
            cv::putText(mdraw, strTxt, cv::Point(x, y), 1, 1.8, cv::Scalar(255, 0, 255, 255), 2);//RGBA
            strTxt = std::string("prob=") + stream_prob.str();
            cv::putText(mdraw, strTxt, cv::Point(x, y+30), 1, 1.8, cv::Scalar(255, 0, 255, 255), 2);//RGBA
        }
 
        // Draw rectangle on target object
        cv::Rect target_rect(rect->x, rect->y, rect->width, rect->height);
        cv::rectangle(mdraw, target_rect, cv::Scalar(0, 255, 0, 255), 2);

        //std::vector<cv::Point> vecCPt;
        VideoPoint *points = inference_result->track;
        cv::Point lastPt, curPt, prePt;
        prePt = cv::Point(points[0].x, points[0].y);
        for (int m = 1; m < inference_result->track_count; m++) {
            curPt = cv::Point(points[m].x, points[m].y);
            if (prePt.y > lastPt.y) {
                lastPt = prePt;
            }
            if (curPt.y > lastPt.y) {
                cv::line(mdraw, lastPt, curPt, cv::Scalar(0, 255, 0, 255), 2);
            }
            prePt = curPt;
        }

        inference_result = inference_result->next;
    }
    if(i!=meta_count) {
        gst_buffer_unref(osd_buf);
        return NULL;
    }

    return osd_buf;
}

GstBuffer* blender_process_cvdl_buffer(BlendHandle handle, GstBuffer* buffer)
{
    CvdlBlender *cvdl_blender  = (CvdlBlender *) handle;
    GstBuffer *osd_buf = NULL, *out_buf = NULL, *dst_buf = NULL;
    VideoRect rect = {0, 0 , (unsigned int)cvdl_blender->mImageWidth,
                             (unsigned int)cvdl_blender->mImageHeight};

    osd_buf = generate_osd(handle, buffer);
    if(!osd_buf) {
        GST_ERROR("Failed to generate osd surface for cvdl buffer!");
        return NULL;
    }

    dst_buf = get_free_buf(handle);
    g_return_val_if_fail(dst_buf, NULL);

    out_buf = dst_buf;
    ImageProcessor *img_processor = static_cast<ImageProcessor *>(cvdl_blender->mImgProcessor);
    img_processor->process_image(osd_buf, buffer, &out_buf, &rect);
    //release osd
    gst_buffer_unref(osd_buf);

    //buffer_test(out_buf);

    // Set cvdl_meta to this buffer
    //cvdl_meta = gst_buffer_get_cvdl_meta(buffer);
    //if(cvdl_meta)
    //    gst_buffer_set_cvdl_meta(out_buf, cvdl_meta);

    g_return_val_if_fail(out_buf, NULL);
    // set pts for it
    GST_BUFFER_TIMESTAMP(out_buf) = GST_BUFFER_TIMESTAMP (buffer);
    GST_BUFFER_DURATION (out_buf) = GST_BUFFER_DURATION (buffer);

    return dst_buf;
}


#ifdef __cplusplus
};
#endif

