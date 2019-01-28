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


#ifndef __OCL_CRC_META_H__
#define __OCL_CRC_META_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "interface/videodefs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
VideoSurfaceID gst_get_mfx_surface(GstBuffer* inbuf, GstVideoInfo *info, VideoDisplayID *display);

#ifdef __cplusplus
};
#endif

#endif
