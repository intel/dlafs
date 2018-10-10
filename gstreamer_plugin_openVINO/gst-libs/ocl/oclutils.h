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
