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

#include "crcmeta.h"
#include "oclcommon.h"

static void crc_meta_free(CrcMeta *meta)
{
    if(!meta)
        return;

    CrcMeta *temp = meta;
    while(meta){
        temp = meta;
        meta=meta->next;
        g_free(temp);
    }
    return;
}

static CrcMeta* crc_meta_create(guint16 x, guint16 y, guint16 width, guint16 height)
{
    CrcMeta *meta = g_slice_new (CrcMeta);

    meta->id = 0;
    meta->crop_x = x;
    meta->crop_x = y;
    meta->crop_width = width;
    meta->crop_height = height;
    meta->next = NULL;

    return meta;
}

static void crc_meta_add(CrcMeta *meta, guint16 x, guint16 y, guint16 width, guint16 height)
{
    if(!meta)
        return;

    guint16 id = 0;
    while(meta->next){
        meta=meta->next;
        id++;
    }

    meta->next = g_slice_new (CrcMeta);
    meta->id = id;
    meta->crop_x = x;
    meta->crop_x = y;
    meta->crop_width = width;
    meta->crop_height = height;
    meta->next = NULL;
    return;
}


static gboolean
crc_meta_holder_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
    CrcMetaHolder *crc_meta_holder = (CrcMetaHolder *) meta;

    crc_meta_holder->buffer = buffer;
    crc_meta_holder->meta   = NULL;

    return TRUE;
}

static void crc_meta_holder_free (GstMeta * meta,
    GstBuffer * buffer)
{
    CrcMetaHolder *crc_meta_holder = (CrcMetaHolder *) meta;
    CrcMeta *crc_meta = crc_meta_holder->meta;

    crc_meta_free(crc_meta);
    g_free(crc_meta_holder);
    return;
}

GType
crc_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = {
    "crc", NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("CrcMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

const GstMetaInfo *
crc_meta_get_info (void)
{
  static const GstMetaInfo *info = NULL;

  if (g_once_init_enter (&info)) {
    const GstMetaInfo *meta =
      gst_meta_register (CRC_META_API_TYPE, "CrcMetaHolder",
        sizeof (CrcMetaHolder),
        crc_meta_holder_init,
        crc_meta_holder_free,
        (GstMetaTransformFunction) NULL);
    g_once_init_leave (&info, meta);
  }
  return info;
}

#define NEW_CRC_META_HOLDER(buf) \
    ((CrcMetaHolder*)gst_buffer_add_meta((buf), CRC_META_INFO, NULL))

gboolean
gst_buffer_add_crc_meta (GstBuffer* buffer, guint16 x, guint16 y, guint16 width, guint16 height)
{
    g_return_val_if_fail (GST_IS_BUFFER (buffer), FALSE);

    CrcMetaHolder *crc_meta_holder = gst_buffer_get_crc_meta_holder (buffer);
    if (!crc_meta_holder) {
        crc_meta_holder = NEW_CRC_META_HOLDER (buffer);
    }

    CrcMeta *crc_meta = crc_meta_holder->meta;

    if(!crc_meta)
        crc_meta_holder->meta = crc_meta_create(x, y, width, height);
    else
        crc_meta_add(crc_meta,x, y, width, height);

    return TRUE;
}

void
gst_buffer_set_crc_meta (GstBuffer * buffer, CrcMeta* meta)
{
  GstMeta *m;

  g_return_if_fail (GST_IS_BUFFER (buffer));
  g_return_if_fail (meta==NULL);

  m = gst_buffer_add_meta (buffer, CRC_META_INFO, NULL);
  ((CrcMetaHolder *)m)->meta = meta;
  ((CrcMetaHolder *)m)->buffer = buffer;
}


CrcMeta *
gst_buffer_get_crc_meta (GstBuffer * buffer)
{
  CrcMeta *meta;
  GstMeta *m;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  m = gst_buffer_get_meta (buffer, CRC_META_API_TYPE);
  if (!m)
    return NULL;

  meta = ((CrcMetaHolder *)m)->meta;
  if (meta)
    ((CrcMetaHolder *)m)->buffer = buffer;
  return meta;
}


OclGstMfxVideoMeta *gst_buffer_get_mfx_meta (GstBuffer * buffer)
{
    OclGstMfxVideoMeta *result = NULL;
    g_return_val_if_fail (buffer != NULL, NULL);

    /* find GstMeta of the requested API */
    gpointer state = NULL;
    GstMeta *meta_item;
    while ((meta_item = gst_buffer_iterate_meta (buffer, &state))) {
        OclGstMfxVideoMeta *meta = (OclGstMfxVideoMeta *)meta_item;
        if (meta->token == 0xFACED) {
            result = meta;
            break;
        }
    }

  return result;
}

VASurfaceID gst_get_mfx_surface(GstBuffer* inbuf, GstVideoInfo *info, VADisplay *display)
{
    VASurfaceID surface = VA_INVALID_SURFACE;

    OclGstMfxVideoMeta *meta = gst_buffer_get_mfx_meta(inbuf);
    g_return_val_if_fail (meta != NULL, VA_INVALID_SURFACE);
    
    surface = (VASurfaceID)meta->surface_id;
    *display = meta->display_id;
    return surface;
}


