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
    gint64 startTimePos;
    gint64 stopTimePos;

    GstTask *mWDTask;
    GRecMutex mWDMutex;
    gboolean mQuited;
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
