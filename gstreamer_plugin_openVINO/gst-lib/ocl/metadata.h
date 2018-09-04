/*
 * Copyright (c) 2017 Intel Corporation
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

#ifndef __META_DATA_H__
#define __META_DATA_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include <glib.h>
#include <va/va.h>

#include "interface/videodefs.h"

#define RGBA_PIXAL_SIZE 4
#define LABEL_MAX_LENGTH 128

#define USE_HOST_PTR_FLAGS    (CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR)
#define ALLLOC_HOST_PTR_FLAGS (CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR)

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _InferenceMeta{
    float     probility;
    VideoRect rect;
    char      label[LABEL_MAX_LENGTH];
    VideoPoint     track[MAX_TRAJECTORY_POINTS_NUM];
    int       track_count;
    guint32   color;
    struct _InferenceMeta *next;
} InferenceMeta;


typedef struct _InferenceMetaHolder{
    GstMeta   base;
    GstBuffer *buffer;
    InferenceMeta *meta;
    int meta_count;
}InferenceMetaHolder;

typedef struct _CvdlMeta{
    VASurfaceID surface_id;
    VADisplay   display_id;
    guint32     width;
    guint32     height;
    InferenceMeta *inference_result;
    int meta_count;
} CvdlMeta;

typedef struct _CvdlMetaHolder{
    GstMeta   base;
    GstBuffer *buffer;
    CvdlMeta  *meta;
}CvdlMetaHolder;


gpointer inference_meta_create (VideoRect *rect, const char *label, float prob, guint32 color);
gpointer inference_meta_add (gpointer meta, VideoRect *rect, const char *label,
                                    float prob, guint32 color, VideoPoint *points, int count);
void inference_meta_free (gpointer meta);


#define INFERENCE_META_API_TYPE  (inference_meta_api_get_type())
#define INFERENCE_META_INFO (inference_meta_get_info())
#define NEW_INFERENCE_META(buf) \
    ((InferenceMeta*)gst_buffer_add_meta (buf, INFERENCE_META_INFO, NULL))

GType inference_meta_api_get_type (void);
void gst_buffer_set_inference_meta (GstBuffer * buffer, InferenceMeta* meta);
InferenceMeta* gst_buffer_get_inference_meta (GstBuffer * buffer);


gpointer
cvdl_meta_create (VADisplay display, VASurfaceID surface, VideoRect *rect, const char *label,
                        float prob, guint32 color, VideoPoint *points, int count);
gpointer
cvdl_meta_add (gpointer meta, VideoRect *rect, const char *label, float prob, guint32 color,
                        VideoPoint *points, int count);
void
cvdl_meta_free (gpointer meta);

#define CVDL_META_API_TYPE  (cvdl_meta_api_get_type())
#define CVDL_META_INFO (cvdl_meta_get_info())
#define NEW_CVDL_META(buf) \
    ((CvdlMeta*)gst_buffer_add_meta (buf, CVDL_META_INFO, NULL))

GType cvdl_meta_api_get_type (void);
void gst_buffer_set_cvdl_meta (GstBuffer * buffer, CvdlMeta* meta);
CvdlMeta *gst_buffer_get_cvdl_meta (GstBuffer * buffer);


#define GST_BUFFER_GET_CVDL_META(buffer) \
    ((CvdlMeta*)gst_buffer_get_meta ((buffer),CVDL_META_API_TYPE))

#ifdef __cplusplus
};
#endif

#endif
