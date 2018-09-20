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
