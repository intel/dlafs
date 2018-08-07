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
#include <string.h>

#include "oclutils.h"
#include "oclcommon.h"
#include "metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

static InferenceMeta* inference_meta_copy(InferenceMeta *meta_src) {
    InferenceMeta *meta_dst = NULL, *temp;

    meta_dst = g_slice_new (InferenceMeta);
    memcpy(meta_dst, meta_src, sizeof(InferenceMeta));

    temp = meta_dst;
    while(meta_src->next) {
        temp->next = g_slice_new (InferenceMeta);
        memcpy(temp->next, meta_src->next, sizeof(InferenceMeta));
        temp = temp->next;
        meta_src = meta_src->next;
    }

    return meta_dst;
}

gpointer
inference_meta_create (VideoRect *rect, const char *label, guint32 color)
{
    InferenceMeta *meta = g_new0 (InferenceMeta, 1);

    meta->rect = *rect;
    memcpy(meta->label,label,LABEL_MAX_LENGTH);
    meta->color = color;
    meta->next = NULL;
    meta->track_count = 1;
    return (gpointer) meta;
}

 gpointer
inference_meta_add (gpointer meta, VideoRect *rect, const char *label, guint32 color)
{
    InferenceMeta *infer_meta = ( InferenceMeta *)meta;

    infer_meta->track_count++;
    while(infer_meta->next)
        infer_meta = infer_meta->next;

    infer_meta->next = (InferenceMeta *)inference_meta_create(rect, label, color);

    return (gpointer) meta;
}

void
inference_meta_free (gpointer meta)
{
    InferenceMeta *infer_meta = ( InferenceMeta *)meta;
    InferenceMeta *temp;

    while(infer_meta) {
        temp = infer_meta;
        infer_meta = infer_meta->next;
        g_free(temp);
    }
    g_free(meta);
    return ;
}

GType
inference_meta_api_get_type (void)
{
    static volatile GType g_type;

    static const gchar *tags[] = {
        "inference",
        "scope",
        "label", NULL };

    if (g_once_init_enter (&g_type)) {
        GType type = gst_meta_api_type_register ("InferenceMetaAPI", tags);
        g_once_init_leave (&g_type, type);
    }
    return g_type;
}

static gboolean
inference_meta_holder_init (GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    InferenceMetaHolder *meta_holder = (InferenceMetaHolder *)meta;
    InferenceMeta *infer_meta = meta_holder->meta;
    if(infer_meta)
        inference_meta_free(infer_meta);
    meta_holder->meta = NULL;
    meta_holder->buffer = buffer;

    return TRUE;
}

static void
inference_meta_holder_free (GstMeta *meta,
    GstBuffer * buffer)
{
  InferenceMetaHolder *meta_holder = (InferenceMetaHolder *)meta;
  if (meta_holder->meta)
    inference_meta_free (meta_holder->meta);
  g_free(meta_holder);
}

static gboolean
inference_meta_holder_transform (GstBuffer * dst_buffer, GstMeta * meta,
    GstBuffer * src_buffer, GQuark type, gpointer data)
{
  InferenceMetaHolder *const src_meta = (InferenceMetaHolder *)meta;

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    InferenceMeta *const dst_meta = inference_meta_copy (src_meta->meta);
    gst_buffer_set_inference_meta (dst_buffer, dst_meta);
    return TRUE;
  }
  return FALSE;
}

static const GstMetaInfo*
inference_meta_get_info (void)
{
    static const GstMetaInfo *meta_info = NULL;

    if (g_once_init_enter (&meta_info)) {
        const GstMetaInfo *mi = gst_meta_register (
            INFERENCE_META_API_TYPE, "InferenceMetaHolder", sizeof (InferenceMetaHolder),
            inference_meta_holder_init, inference_meta_holder_free, inference_meta_holder_transform);
        g_once_init_leave (&meta_info, mi);
    }

    return meta_info;
}


void
gst_buffer_set_inference_meta (GstBuffer * buffer, InferenceMeta* meta)
{
  GstMeta *m;

  g_return_if_fail (GST_IS_BUFFER (buffer));
  g_return_if_fail (meta==NULL);

  m = gst_buffer_add_meta (buffer, INFERENCE_META_INFO, NULL);
  ((InferenceMetaHolder *)m)->meta = meta;
  ((InferenceMetaHolder *)m)->buffer = buffer;
}


InferenceMeta *
gst_buffer_get_inference_meta (GstBuffer * buffer)
{
  InferenceMeta *meta;
  GstMeta *m;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  m = gst_buffer_get_meta (buffer, INFERENCE_META_API_TYPE);
  if (!m)
    return NULL;

  meta = ((InferenceMetaHolder *)m)->meta;
  if (meta)
    ((InferenceMetaHolder *)m)->buffer = buffer;
  return meta;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// cvdl meta data
//
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* called when allocate cvdlfilter output buffer*/
gpointer
cvdl_meta_create (VADisplay display, VASurfaceID surface, VideoRect *rect, const char *label, guint32 color)
{
    CvdlMeta *meta = g_new0 (CvdlMeta, 1);

    meta->surface_id = surface;
    meta->display_id = display;
    meta->inference_result = (InferenceMeta *)inference_meta_create(rect, label, color);

    return (gpointer) meta;
}


gpointer
cvdl_meta_add (gpointer meta, VideoRect *rect, const char *label, guint32 color)
{
    //CvdlMeta *cvdl_meta = ( CvdlMeta *)meta;

    //inference_meta_add((gpointer)cvdl_meta->inference_result);
    inference_meta_add(meta, rect, label, color);
    return (gpointer) meta;
}

void
cvdl_meta_free (gpointer meta)
{
    CvdlMeta *cvdl_meta = ( CvdlMeta *)meta;

    inference_meta_free ((gpointer)cvdl_meta->inference_result);
    g_free(meta);
    return ;
}


static CvdlMeta*
cvdl_meta_copy(CvdlMeta *meta_src) {
    CvdlMeta *meta_dst = NULL;

    meta_dst = g_slice_new (CvdlMeta);
    memcpy(meta_dst, meta_src, sizeof(CvdlMeta));
    meta_dst->inference_result = inference_meta_copy(meta_src->inference_result);

    return meta_dst;
}


GType
cvdl_meta_api_get_type (void)
{
    static volatile GType g_type;

    static const gchar *tags[] = {
        "surface",
        "inference",
        "display", NULL };

    if (g_once_init_enter (&g_type)) {
        GType type = gst_meta_api_type_register ("CVDLMetaAPI", tags);
        g_once_init_leave (&g_type, type);
    }
    return g_type;
}


static gboolean
cvdl_meta_holder_init (GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    CvdlMetaHolder *meta_holder = (CvdlMetaHolder *)meta;
    CvdlMeta *cvdl_meta = meta_holder->meta;
    if(cvdl_meta)
        cvdl_meta_free(cvdl_meta);
    meta_holder->meta = NULL;
    meta_holder->buffer = buffer;
    return TRUE;
}

static void
cvdl_meta_holder_free (GstMeta * meta,
    GstBuffer * buffer)
{
    CvdlMetaHolder *meta_holder = (CvdlMetaHolder *)meta;
    if (meta_holder->meta)
        cvdl_meta_free (meta_holder->meta);
    meta_holder->meta = NULL;
    g_free(meta_holder);
}

static gboolean
cvdl_meta_holder_transform (GstBuffer * dst_buffer, GstMeta * meta,
    GstBuffer * src_buffer, GQuark type, gpointer data)
{
  CvdlMetaHolder *const src_meta = (CvdlMetaHolder *const)meta;

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    CvdlMeta *dst_meta = cvdl_meta_copy (src_meta->meta);
    gst_buffer_set_cvdl_meta (dst_buffer, dst_meta);
    return TRUE;
  }
  return FALSE;
}


static const GstMetaInfo*
cvdl_meta_get_info (void)
{
    static const GstMetaInfo *meta_info = NULL;

    if (g_once_init_enter (&meta_info)) {
        const GstMetaInfo *mi = gst_meta_register (
            INFERENCE_META_API_TYPE, "CVDLMetaHolder", sizeof (CvdlMetaHolder),
            cvdl_meta_holder_init, cvdl_meta_holder_free, cvdl_meta_holder_transform);
        g_once_init_leave (&meta_info, mi);
    }

    return meta_info;
}


void
gst_buffer_set_cvdl_meta (GstBuffer * buffer, CvdlMeta* meta)
{
  GstMeta *m;

  g_return_if_fail (GST_IS_BUFFER (buffer));
  g_return_if_fail (meta==NULL);

  m = gst_buffer_add_meta (buffer, CVDL_META_INFO, NULL);
  ((CvdlMetaHolder *)m)->meta = meta;
  ((CvdlMetaHolder *)m)->buffer = buffer;
}


CvdlMeta *
gst_buffer_get_cvdl_meta (GstBuffer * buffer)
{
  CvdlMeta *meta;
  GstMeta *m;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  m = gst_buffer_get_meta (buffer, CVDL_META_API_TYPE);
  if (!m)
    return NULL;

  meta = ((CvdlMetaHolder *)m)->meta;
  if (meta)
    ((CvdlMetaHolder *)m)->buffer = buffer;
  return meta;
}

#ifdef __cplusplus
};
#endif

