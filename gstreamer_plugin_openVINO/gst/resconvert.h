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


#ifndef __GST_RES_CONVERT_H__
#define __GST_RES_CONVERT_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstflowcombiner.h>
#include <gst/video/video-info.h>
#include <algo/blendwrapper.h>

G_BEGIN_DECLS

#define TYPE_RES_CONVERT		    (res_convert_get_type())
#define RES_CONVERT(obj)		    (G_TYPE_CHECK_INSTANCE_CAST((obj),TYPE_RES_CONVERT,ResConvert))
#define RES_CONVERT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),TYPE_RES_CONVERT,ResConvertClass))
#define RES_CONVERT_GET_CLASS(klass)(G_TYPE_INSTANCE_GET_CLASS((klass),TYPE_RES_CONVERT,ResConvertClass))
#define IS_RES_CONVERT(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE((obj),TYPE_RES_CONVERT))
#define IS_RES_CONVERT_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE((klass),TYPE_RES_CONVERT))

typedef struct _ResConvert ResConvert;
typedef struct _ResConvertClass ResConvertClass;
typedef struct _ResOclBlendPrivate  ResOclBlendPrivate;


struct _ResConvert
{
  GstElement parent;

  GstPad *sinkpad;
  GstPad *txt_srcpad;     /* output inference result raw data */
  GstPad *pic_srcpad;     /* output inference result picture data */

  GstBufferPool* src_pool;
  GstBufferPool* osd_pool;

  GstVideoInfo     sink_info;
  GstVideoInfo     src_info;

  ResOclBlendPrivate *priv;
  BlendHandle blend_handle;

  gint64 cost_ms;
  gint64 frame_num;
};

struct _ResConvertClass
{
  GstElementClass parent_class;

  GstPadTemplate *sink_template;
  GstPadTemplate *src_txt_template;
  GstPadTemplate *src_pic_template;
};

GType res_convert_get_type (void);

G_END_DECLS
#endif /* __GST_PS_DEMUX_H__ */
