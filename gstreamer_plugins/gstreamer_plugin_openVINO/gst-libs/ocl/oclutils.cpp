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

#include "interface/videodefs.h"
#include "oclutils.h"

guint
video_format_to_va_fourcc (GstVideoFormat format)
{
    switch(format) {
        case GST_VIDEO_FORMAT_NV12: return OCL_FOURCC_NV12;
        case GST_VIDEO_FORMAT_I420: return OCL_FOURCC_YV12;
        case GST_VIDEO_FORMAT_YV12: return OCL_FOURCC_YV12;
        case GST_VIDEO_FORMAT_YUY2: return OCL_FOURCC_YUY2;
        case GST_VIDEO_FORMAT_UYVY: return OCL_FOURCC_UYVY;
        case GST_VIDEO_FORMAT_BGRA: return OCL_FOURCC_BGRA;
        case GST_VIDEO_FORMAT_BGRx: return OCL_FOURCC_BGRX;
        case GST_VIDEO_FORMAT_BGR:  return OCL_FOURCC_BGR3;
        default: return 0;
    }
}

gboolean
ocl_video_rect_set (VideoRect* dest, VideoRect* src)
{
    if (!dest)
        return FALSE;

    if (src) {
        dest->x = src->x;
        dest->y = src->y;
        dest->width = src->width;
        dest->height = src->height;
    } else {
        dest->x = 0;
        dest->y = 0;
        dest->width = 0;
        dest->height = 0;
    }

    return TRUE;
}

gboolean
ocl_compare_caps (GstCaps* incaps, GstCaps* outcaps)
{
    if (!incaps || !outcaps)
        return FALSE;

    GstStructure *s1 = gst_caps_get_structure (incaps, 0);
    GstStructure *s2 = gst_caps_get_structure (outcaps, 0);

    GstCapsFeatures *f1 = gst_caps_get_features (incaps, 0);
    if (!f1) {
        f1 = GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY;
    }

    GstCapsFeatures *f2 = gst_caps_get_features (outcaps, 0);
    if (!f2) {
        f2 = GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY;
    }

    if (!gst_caps_features_is_equal (f1, f2)) {
        return FALSE;
    }

    if (g_ascii_strcasecmp (gst_structure_get_name (s1), gst_structure_get_name (s2))) {
        return FALSE;
    }

    if (gst_value_compare (gst_structure_get_value (s1, "format"),
                           gst_structure_get_value (s2, "format")) ||
        gst_value_compare (gst_structure_get_value (s1, "width"),
                           gst_structure_get_value (s2, "width")) ||
        gst_value_compare (gst_structure_get_value (s1, "height"),
                           gst_structure_get_value (s2, "height"))) {
        return FALSE;
    }

    return TRUE;
}

gboolean
ocl_fixate_caps (GstCaps* caps)
{
    return TRUE;
}

gboolean have_mfx_feature(GstCaps* caps)
{
    gchar * feature_str = GET_FEATRUE_STRING (caps);
    gboolean value =  ! g_strcmp0 (MFX_FEATURE_STRING, feature_str);
    g_free (feature_str);
    return value;

}
