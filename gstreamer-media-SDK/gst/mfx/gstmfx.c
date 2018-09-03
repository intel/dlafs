/*
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Ishmael Visayana Sameen <ishmael.visayana.sameen@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "version.h"

#ifdef MFX_DECODER
#include "gstmfxdec.h"
#endif
#ifdef MFX_VPP
# include "gstmfxpostproc.h"
#endif
#ifdef MFX_SINK
# include "gstmfxsink.h"
#endif
#ifdef MFX_SINK_BIN
# include "gstmfxsinkbin.h"
#endif
#ifdef MFX_H264_ENCODER
# include "gstmfxenc_h264.h"
#endif
#ifdef MFX_H265_ENCODER
# include "gstmfxenc_h265.h"
#endif
#ifdef MFX_MPEG2_ENCODER
# include "gstmfxenc_mpeg2.h"
#endif
#ifdef MFX_JPEG_ENCODER
# include "gstmfxenc_jpeg.h"
#endif

#ifdef MFX_VC1_PARSER
# include "parsers/gstvc1parse.h"
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;

#ifdef MFX_DECODER
  ret |= gst_mfxdec_register (plugin);
#endif

#ifdef MFX_VPP
  ret |= gst_element_register (plugin, "mfxvpp",
      GST_RANK_NONE, GST_TYPE_MFXPOSTPROC);
#endif

#ifdef MFX_SINK
  ret |= gst_element_register (plugin, "mfxsinkelement",
      GST_RANK_NONE, GST_TYPE_MFXSINK);
#endif

#ifdef MFX_SINK_BIN
  ret |= gst_element_register (plugin, "mfxsink",
      GST_RANK_PRIMARY + 2, GST_TYPE_MFX_SINK_BIN);
#endif

#ifdef MFX_H264_ENCODER
  ret |= gst_element_register (plugin, "mfxh264enc",
      GST_RANK_NONE, GST_TYPE_MFXENC_H264);
#endif

#ifdef MFX_H265_ENCODER
  ret |= gst_element_register (plugin, "mfxhevcenc",
      GST_RANK_NONE, GST_TYPE_MFXENC_H265);
#endif

#ifdef MFX_MPEG2_ENCODER
  ret |= gst_element_register (plugin, "mfxmpeg2enc",
      GST_RANK_NONE, GST_TYPE_MFXENC_MPEG2);
#endif

#ifdef MFX_JPEG_ENCODER
  ret |= gst_element_register (plugin, "mfxjpegenc",
      GST_RANK_NONE, GST_TYPE_MFXENC_JPEG);
#endif

#ifdef MFX_VC1_PARSER
  ret |= gst_element_register (plugin, "mfxvc1parse",
      GST_RANK_MARGINAL, GST_MFX_TYPE_VC1_PARSE);
#endif

  return ret;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,   /* major */
    GST_VERSION_MINOR,          /* minor */
    mfx,                        /* short unique name */
    "MFX encoder/decoder/video post-processing plugins",        /* info */
    plugin_init,                /* GstPlugin::plugin_init */
    PACKAGE_VERSION,            /* version */
    "LGPL",                     /* license */
    PACKAGE,                    /* package-name, usually the file archive name */
    "http://www.intel.com"      /* origin */
    )
