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

#include <gst/gst.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cvdlfilter.h"

//GST_DEBUG_CATEGORY_EXTERN (cvdl_filter_debug);

static gboolean
plugin_init (GstPlugin * plugin)
{
  //GST_DEBUG_CATEGORY_INIT (cvdl_filter_debug, "cvdlfilter", 0,
  //    "CVDL filter");

  if (!gst_element_register (plugin, "cvdlfilter", GST_RANK_PRIMARY,
          CVDL_FILTER_TYPE))
    return FALSE;

  return TRUE;
}

#ifndef VERSION
#define VERSION "1.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "GStreamer"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "GStreamer-CVDL"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://www.intel.com"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cvdlfilter,
    "CVDL filter",
    plugin_init, VERSION,
    "BSD", PACKAGE_NAME, GST_PACKAGE_ORIGIN);
