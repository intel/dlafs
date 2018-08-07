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

#ifndef __OCL_CRC_META_H__
#define __OCL_CRC_META_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "interface/videodefs.h"

typedef struct _CrcMeta CrcMeta;
struct _CrcMeta {
  guint16  id;
  guint16  crop_x;
  guint16  crop_y;
  guint16  crop_width;
  guint16  crop_height;
  CrcMeta *next;
};

typedef struct _CrcMetaHolder{
    GstMeta base;
    GstBuffer *buffer;
    CrcMeta   *meta;
}CrcMetaHolder;

GType crc_meta_api_get_type (void);
const GstMetaInfo * crc_meta_get_info (void);

#define CRC_META_API_TYPE  (crc_meta_api_get_type())
#define CRC_META_INFO  (crc_meta_get_info())


#define gst_buffer_get_crc_meta_holder(b) ((CrcMetaHolder *)gst_buffer_get_meta((b), CRC_META_API_TYPE))
//#define gst_buffer_get_crc_meta(b) (((CrcMetaHolder *)gst_buffer_get_meta((b), CRC_META_API_TYPE))->meta)


gboolean gst_buffer_add_crc_meta (GstBuffer* buffer, guint16 x, guint16 y, guint16 width, guint16 height);
void gst_buffer_set_crc_meta (GstBuffer * buffer, CrcMeta* meta);
CrcMeta* gst_buffer_get_crc_meta_ (GstBuffer * buffer);


OclGstMfxVideoMeta *gst_buffer_get_mfx_meta (GstBuffer * buffer);
VASurfaceID gst_get_mfx_surface(GstBuffer* inbuf, GstVideoInfo *info, VADisplay *display);

#endif
