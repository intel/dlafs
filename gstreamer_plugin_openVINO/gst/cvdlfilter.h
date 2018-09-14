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

#ifndef __CVDL_FILTER_H__
#define __CVDL_FILTER_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>
#include "algo/algopipeline.h"


G_BEGIN_DECLS

#define CVDL_FILTER_TYPE \
    (cvdl_filter_get_type())
#define CVDL_FILTER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),CVDL_FILTER_TYPE,CvdlFilter))
#define CVDL_FILTER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass),CVDL_FILTER_TYPE,CvdlFilterClass))
#define IS_CVDL_FILTER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),CVDL_FILTER_TYPE))
#define IS_CVDL_FILTER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass),CVDL_FILTER_TYPE))

typedef struct _CvdlFilter        CvdlFilter;
typedef struct _CvdlFilterClass   CvdlFilterClass;
typedef struct _CvdlFilterPrivate CvdlFilterPrivate;

struct _CvdlFilter
{
    GstBaseTransform    element;
    gboolean            negotiated;
    gboolean            same_caps_flag;

    GstVideoInfo        sink_info;
    GstVideoInfo        src_info;

    AlgoPipelineHandle algoHandle;

    CvdlFilterPrivate*  priv;
    gchar* algo_pipeline_desc;

    GstTask *mPushTask;
    GRecMutex mMutex;
    int frame_num;

    // debug
    int frame_num_in;
    int frame_num_out;
};

struct _CvdlFilterClass
{
  GstBaseTransformClass parent_class;
};

GType cvdl_filter_get_type (void);

G_END_DECLS
#endif /* __CVDL_FILTER_H__ */
