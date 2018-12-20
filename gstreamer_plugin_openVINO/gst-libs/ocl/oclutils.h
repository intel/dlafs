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

#ifndef __OCL_UTILS_H__
#define __OCL_UTILS_H__

#include <gst/video/video.h>

#include "interface/videodefs.h"

G_BEGIN_DECLS

const char ocl_sink_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("NV12") "; " \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:MFXSurface", "NV12");

const char ocl_osd_sink_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("RGBA");

const char ocl_src_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("NV12") "; " \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:MFXSurface", "NV12");

const char scale_src_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("{ NV12, xBGR }") "; " \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:MFXSurface", "{ NV12, xBGR }");

#define MFX_FEATURE_STRING     "memory:MFXSurface"
#define GET_FEATRUE_STRING(caps) gst_caps_features_to_string (gst_caps_get_features (caps, 0))
#define HAVE_MFX_FEATURE(caps)   (have_mfx_feature(caps))

#define PRINT_BUFFER_ERROR(plugin) GST_ELEMENT_ERROR(plugin, RESOURCE, WRITE, \
    ("Internal error: can't get a buffer from buffer pool"), \
    ("We don't have a OclMemory"))
#define PRINT_SURFACE_MAPPING_ERROR(plugin) GST_ELEMENT_ERROR(plugin, RESOURCE, WRITE, \
    ("Internal error: can't get a mapped surface from outbuf"), \
    ("We can't create a mapped surface"))

guint
video_format_to_va_fourcc (GstVideoFormat format);

gboolean
ocl_video_rect_set (VideoRect* dest, VideoRect* src = NULL);

gboolean
ocl_compare_caps (GstCaps* incaps, GstCaps* outcaps);

gboolean
ocl_fixate_caps (GstCaps* caps);

gboolean
have_mfx_feature (GstCaps* caps);

gboolean
gst_buffer_transfer_osd_meta (GstBuffer *srcBuf, GstBuffer* dstBuf);

G_END_DECLS

#endif /* __OCL_UTILS_H__ */
