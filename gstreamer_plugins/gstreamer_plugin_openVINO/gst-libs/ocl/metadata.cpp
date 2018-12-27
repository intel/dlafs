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

#include <string.h>
#include "oclutils.h"
#include "oclcommon.h"
#include "metadata.h"

using namespace std;
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

static InferenceMeta* inference_meta_copy(InferenceMeta *meta_src) {
    InferenceMeta *meta_dst = NULL, *temp;

    meta_dst = g_slice_new (InferenceMeta);
    //memcpy(meta_dst, meta_src, sizeof(InferenceMeta));
    std::copy(meta_src, meta_src+1, meta_dst);

    temp = meta_dst;
    while(meta_src->next) {
        temp->next = g_slice_new (InferenceMeta);
        //memcpy(temp->next, meta_src->next, sizeof(InferenceMeta));
        std::copy(meta_src->next, meta_src->next+1, temp->next);
        temp = temp->next;
        meta_src = meta_src->next;
    }

    return meta_dst;
}

gpointer
inference_meta_create (VideoRect *rect, const char *label, float prob, guint32 color)
{
    InferenceMeta *meta = g_new0 (InferenceMeta, 1);
    int len = std::string(label).size();
    if(len>=LABEL_MAX_LENGTH)
        len = LABEL_MAX_LENGTH - 1;

    meta->rect = *rect;
    //memcpy(meta->label,label,len);
    std::copy(label, label+len, meta->label);
    meta->color = color;
    meta->next = NULL;
    meta->track_count = 1;
    meta->probility = prob;
    return (gpointer) meta;
}

 gpointer
inference_meta_add (gpointer meta, VideoRect *rect, const char *label,
                            float prob, guint32 color,
                            VideoPoint *points, int count)
{
    InferenceMeta *infer_meta = ( InferenceMeta *)meta;

    //infer_meta->track_count++;
    while(infer_meta->next)
        infer_meta = infer_meta->next;

    InferenceMeta *new_infer_meta =
        (InferenceMeta *)inference_meta_create(rect, label, prob, color);

    for(int i=0; i<count; i++) {
        new_infer_meta->track[i] = points[i];
    }
    new_infer_meta->track_count = count;

    infer_meta->next = new_infer_meta;
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
  // Don't free it due to it will be freed in gst_buffer_foreach_meta
  // g_free(meta_holder);
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
            inference_meta_holder_init, inference_meta_holder_free,
            inference_meta_holder_transform);
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

//------------------------------------------------------------------------------------------
//
// cvdl meta data
//
//-------------------------------------------------------------------------------------------

/* called when allocate cvdlfilter output buffer*/
gpointer
cvdl_meta_create (VADisplay display, VASurfaceID surface,
                       VideoRect *rect, const char *label,
                       float prob, guint32 color,
                       VideoPoint *points, int count)
{
    CvdlMeta *meta = g_new0 (CvdlMeta, 1);

    meta->surface_id = surface;
    meta->display_id = display;
    meta->inference_result =
        (InferenceMeta *)inference_meta_create(rect, label, prob, color);

    for(int i=0; i<count; i++) {
        meta->inference_result->track[i] = points[i];
    }
    meta->inference_result->track_count = count;
    return (gpointer) meta;
}


gpointer
cvdl_meta_add (gpointer meta, VideoRect *rect, const char *label,
                    float prob, guint32 color, VideoPoint *points, int count)
{
    CvdlMeta *cvdl_meta = ( CvdlMeta *)meta;

    //inference_meta_add((gpointer)cvdl_meta->inference_result);
    inference_meta_add(cvdl_meta->inference_result, rect, label, prob, color, points, count);
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
    //memcpy(meta_dst, meta_src, sizeof(CvdlMeta));
    std::copy(meta_src, meta_src+1, meta_dst);
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
    //CvdlMeta *cvdl_meta = meta_holder->meta;

    // It will be called by gst_buffer_add_meta()
    // meta is allocated just now, and it was not inited so far 
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

    // Don't free it due to it will be freed in gst_buffer_foreach_meta
    //g_free(meta_holder);
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
            CVDL_META_API_TYPE, "CVDLMetaHolder", sizeof (CvdlMetaHolder),
            cvdl_meta_holder_init, cvdl_meta_holder_free, cvdl_meta_holder_transform);
        g_once_init_leave (&meta_info, mi);
    }

    return meta_info;
}


void
gst_buffer_set_cvdl_meta (GstBuffer * buffer, CvdlMeta* meta)
{
  GstMeta *m = NULL;
  int count = 0;

  g_return_if_fail (GST_IS_BUFFER (buffer));
  g_return_if_fail (meta!=NULL);

  /* Use this function to ensure that a buffer can be safely modified before
    * making changes to it, including changing the metadata such as PTS/DTS.
    *
    * If the reference count of the source buffer @buf is exactly one, the caller
    * is the sole owner and this function will return the buffer object unchanged.
    *
    * If there is more than one reference on the object, a copy will be made using
    * gst_buffer_copy(). The passed-in @buf will be unreffed in that case, and the
    * caller will now own a reference to the new returned buffer object. Note
    * that this just copies the buffer structure itself, the underlying memory is
    * not copied if it can be shared amongst multiple buffers.
    *
    * In short, this function unrefs the buf in the argument and refs the buffer
    * that it returns. Don't access the argument after calling this function unless
    * you have an additional reference to it.
    */

  // make this buffer to be writable
  if(GST_MINI_OBJECT_REFCOUNT(buffer)>=2) {
    gst_buffer_unref(buffer);
    count = 1;
  }
  m = gst_buffer_add_meta (buffer, CVDL_META_INFO, NULL);
  if(count == 1) {
    buffer = gst_buffer_ref(buffer);
  }
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

