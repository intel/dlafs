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

#include "va/va.h"
#include "oclutils.h"

guint
video_format_to_va_fourcc (GstVideoFormat format)
{
    switch(format) {
        case GST_VIDEO_FORMAT_NV12: return VA_FOURCC_NV12;
        case GST_VIDEO_FORMAT_I420: return VA_FOURCC_YV12;
        case GST_VIDEO_FORMAT_YV12: return VA_FOURCC_YV12;
        case GST_VIDEO_FORMAT_YUY2: return VA_FOURCC_YUY2;
        case GST_VIDEO_FORMAT_UYVY: return VA_FOURCC_UYVY;
        case GST_VIDEO_FORMAT_BGRA: return VA_FOURCC_ARGB;
        case GST_VIDEO_FORMAT_BGRx: return VA_FOURCC_ARGB;
        case GST_VIDEO_FORMAT_BGR:  return VA_FOURCC_ARGB;
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
