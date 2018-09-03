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

#include "sysdeps.h"
#include "common/gstbitwriter.h"
#include "gstmfxencoder_priv.h"
#include "gstmfxencoder_h265.h"
#include <mfxplugin.h>

#define DEBUG 1
#include "gstmfxdebug.h"

/* Define default rate control mode ("constant-qp") */
#define DEFAULT_RATECONTROL GST_MFX_RATECONTROL_CQP

/* Supported set of rate control methods, within this implementation */
#define SUPPORTED_RATECONTROLS                      \
  (GST_MFX_RATECONTROL_MASK (CQP)     |             \
  GST_MFX_RATECONTROL_MASK (CBR)      |             \
  GST_MFX_RATECONTROL_MASK (VBR)      |             \
  GST_MFX_RATECONTROL_MASK (AVBR)     |             \
  GST_MFX_RATECONTROL_MASK (VCM)      |             \
  GST_MFX_RATECONTROL_MASK (LA_BRC)   |             \
  GST_MFX_RATECONTROL_MASK (LA_HRD)   |             \
  GST_MFX_RATECONTROL_MASK (ICQ)      |             \
  GST_MFX_RATECONTROL_MASK (LA_ICQ))

/* ------------------------------------------------------------------------- */
/* --- H.265 Bitstream Writer                                            --- */
/* ------------------------------------------------------------------------- */

#define WRITE_UINT32(bs, val, nbits) do {                           \
    if (!gst_bit_writer_put_bits_uint32 (bs, val, nbits)) {         \
        GST_WARNING ("failed to write uint32, nbits: %d", nbits);   \
        goto bs_error;                                              \
    }                                                               \
  } while (0)

/* ------------------------------------------------------------------------- */
/* --- H.265 Encoder                                                     --- */
/* ------------------------------------------------------------------------- */

#define GST_MFX_ENCODER_H265_CAST(encoder) \
  ((GstMfxEncoderH265 *)(encoder))

struct _GstMfxEncoderH265
{
  GstMfxEncoder parent_instance;
};

/* Estimates a good enough bitrate if none was supplied */
static void
ensure_bitrate (GstMfxEncoderH265 * encoder)
{
  GstMfxEncoder *const base_encoder = GST_MFX_ENCODER_CAST (encoder);

  switch (GST_MFX_ENCODER_RATE_CONTROL (encoder)) {
    case GST_MFX_RATECONTROL_CBR:
    case GST_MFX_RATECONTROL_VBR:
    case GST_MFX_RATECONTROL_AVBR:
    case GST_MFX_RATECONTROL_VCM:
    case GST_MFX_RATECONTROL_LA_BRC:
    case GST_MFX_RATECONTROL_LA_HRD:
      if (!base_encoder->bitrate) {
        guint luma_width = GST_ROUND_UP_32 (GST_MFX_ENCODER_WIDTH (encoder));
        guint luma_height = GST_ROUND_UP_32 (GST_MFX_ENCODER_HEIGHT (encoder));

        /* Fixme: Provide better estimation */
        /* Using a 1/6 compression ratio */
        /* 12 bits per pixel for yuv420 */
        base_encoder->bitrate =
            (luma_width * luma_height * 12 / 6) *
            GST_MFX_ENCODER_FPS_N (encoder) /
            GST_MFX_ENCODER_FPS_D (encoder) / 1000;
        GST_INFO ("target bitrate computed to %u kbps", base_encoder->bitrate);
      }
      break;
    default:
      base_encoder->bitrate = 0;
      break;
  }
}

static GstMfxEncoderStatus
gst_mfx_encoder_h265_reconfigure (GstMfxEncoder * base_encoder)
{
  GstMfxEncoderH265 *const encoder = GST_MFX_ENCODER_H265_CAST (base_encoder);

  GST_DEBUG ("resolution: %dx%d", GST_MFX_ENCODER_WIDTH (encoder),
      GST_MFX_ENCODER_HEIGHT (encoder));

  /* Ensure bitrate if not set */
  ensure_bitrate (encoder);

  return GST_MFX_ENCODER_STATUS_SUCCESS;
}

static mfxStatus
gst_mfx_encoder_load_hevc_plugin (GstMfxEncoder * encoder)
{
  mfxPluginUID uid;
  mfxStatus sts;
  guint i, c;

  gchar *plugin_uids[] = {
    "6fadc791a0c2eb479ab6dcd5ea9da347",     /* HW encoder */
    //"e5400a06c74d41f5b12d430bbaa23d0b",   /* GPU-accelerated encoder */
    "2fca99749fdb49aeb121a5b63ef568f7",     /* SW decoder */
    NULL
  };
  for (i = 0; plugin_uids[i]; i++) {
    for (c = 0; c < sizeof (uid.Data); c++)
      sscanf (plugin_uids[i] + 2 * c, "%2hhx", uid.Data + c);
    sts = MFXVideoUSER_Load (encoder->session, &uid, 1);
    if (MFX_ERR_NONE == sts) {
      encoder->plugin_uid = g_strdup (plugin_uids[i]);
      GST_DEBUG ("Loaded HEVC encoder plugin %s", encoder->plugin_uid);
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
gst_mfx_encoder_h265_init (GstMfxEncoder * base_encoder)
{
  base_encoder->codec = MFX_CODEC_HEVC;

  return gst_mfx_encoder_load_hevc_plugin (base_encoder);
}

static void
gst_mfx_encoder_h265_finalize (GstMfxEncoder * base_encoder)
{
  mfxPluginUID uid;
  guint c;

  for (c = 0; c < sizeof (uid.Data); c++)
    sscanf (base_encoder->plugin_uid + 2 * c, "%2hhx", uid.Data + c);

  MFXVideoUSER_UnLoad (base_encoder->session, &uid);
  g_free (base_encoder->plugin_uid);
}

/* Generate "codec-data" buffer */
static GstMfxEncoderStatus
gst_mfx_encoder_h265_get_codec_data (GstMfxEncoder * base_encoder,
    GstBuffer ** out_buffer_ptr)
{
  GstBuffer *buffer;
  mfxStatus sts;
  guint8 sps_data[128], pps_data[128], vps_data[128];
  guint8 *sps_info, *pps_info, *vps_info;
  guint sps_size, pps_size, vps_size;
  GstBitWriter bs;
  mfxFrameInfo *frame_info;

  const guint configuration_version = 0x01;
  const guint nal_length_size = 4;
  const guint min_spatial_segmentation_idc = 0;
  const guint num_arrays = 3;

  mfxExtCodingOptionVPS vps = {
    .Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS,
    .Header.BufferSz = sizeof (vps),
    .VPSBuffer = vps_data,.VPSBufSize = sizeof (vps_data)
  };

  mfxExtCodingOptionSPSPPS spspps = {
    .Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS,
    .Header.BufferSz = sizeof (spspps),
    .SPSBuffer = sps_data,.SPSBufSize = sizeof (sps_data),
    .PPSBuffer = pps_data,.PPSBufSize = sizeof (pps_data)
  };

  mfxExtBuffer *ext_buffers[] = {
    (mfxExtBuffer *) &spspps,
    (mfxExtBuffer *) &vps,
  };

  base_encoder->params.ExtParam = ext_buffers;
  base_encoder->params.NumExtParam = 2;

  sts = MFXVideoENCODE_GetVideoParam (base_encoder->session,
      &base_encoder->params);
  if (sts < 0)
    return GST_MFX_ENCODER_STATUS_ERROR_INVALID_PARAMETER;

  frame_info = &base_encoder->params.mfx.FrameInfo;

  const guint8 general_profile_space = 0;
  const guint8 general_tier_flag =
      !!(base_encoder->params.mfx.CodecLevel & MFX_TIER_HEVC_HIGH);
  const guint8 general_profile_idc = base_encoder->params.mfx.CodecProfile;
  const guint32 general_profile_compatibility_flags =
      1 << (31 - general_profile_idc);
  const guint8 progressive_source_flag =
      !!(frame_info->PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
  const guint8 interlaced_source_flag =
      !!(frame_info->PicStruct &
      (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF));
  const guint8 non_packed_constraint_flag = 0;
  const guint8 frame_only_constraint_flag = 0;
  const guint8 general_level_idc =
      (base_encoder->params.mfx.CodecLevel & 0xFF) * 3;
  const guint8 chroma_format_idc = frame_info->ChromaFormat;

  sps_info = &sps_data[4];
  pps_info = &pps_data[4];
  vps_info = &vps_data[4];
  sps_size = spspps.SPSBufSize - 4;
  pps_size = spspps.PPSBufSize - 4;
  vps_size = vps.VPSBufSize - 4;

  /* Header */
  gst_bit_writer_init (&bs, (vps_size + sps_size + pps_size + 64) * 8);
  WRITE_UINT32 (&bs, configuration_version, 8);

  /* profile_space | tier_flag | profile_idc */
  WRITE_UINT32 (&bs, general_profile_space, 2);
  WRITE_UINT32 (&bs, general_tier_flag, 1);
  WRITE_UINT32 (&bs, general_profile_idc, 5);

  /* profile_compatibility_flag [0-31] */
  WRITE_UINT32 (&bs, (general_profile_compatibility_flags >> 8), 24);
  WRITE_UINT32 (&bs, (general_profile_compatibility_flags & 0xFF), 8);
  /* progressive_source_flag | interlaced_source_flag | non_packed_constraint_flag |
   * frame_only_constraint_flag | reserved_zero_bits[0-27] */
  WRITE_UINT32 (&bs, progressive_source_flag, 1);
  WRITE_UINT32 (&bs, interlaced_source_flag, 1);
  WRITE_UINT32 (&bs, non_packed_constraint_flag, 1);
  WRITE_UINT32 (&bs, frame_only_constraint_flag, 1);
  WRITE_UINT32 (&bs, 0x00, 24);
  WRITE_UINT32 (&bs, 0x00, 20);

  WRITE_UINT32 (&bs, general_level_idc, 8);     /* level_idc */

  WRITE_UINT32 (&bs, 0x0f, 4);  /* 1111 */
  WRITE_UINT32 (&bs, min_spatial_segmentation_idc, 12); /* min_spatial_segmentation_idc */
  WRITE_UINT32 (&bs, 0x3f, 6);  /* 111111 */
  WRITE_UINT32 (&bs, 0x00, 2);  /* parallelismType */
  WRITE_UINT32 (&bs, 0x3f, 6);  /* 111111 */

  WRITE_UINT32 (&bs, chroma_format_idc, 2);     /* chroma_format_idc */

  WRITE_UINT32 (&bs, 0x1f, 5);  /* 11111 */
  WRITE_UINT32 (&bs, 0x01, 3);  /* bit_depth_luma_minus8 */
  WRITE_UINT32 (&bs, 0x1f, 5);  /* 11111 */
  WRITE_UINT32 (&bs, 0x01, 3);  /* bit_depth_chroma_minus8 */
  WRITE_UINT32 (&bs, 0x00, 16); /* avgFramerate */
  WRITE_UINT32 (&bs, 0x00, 2);  /* constantFramerate */
  WRITE_UINT32 (&bs, 0x00, 3);  /* numTemporalLayers */
  WRITE_UINT32 (&bs, 0x00, 1);  /* temporalIdNested */
  WRITE_UINT32 (&bs, nal_length_size - 1, 2);   /* lengthSizeMinusOne */

  WRITE_UINT32 (&bs, num_arrays, 8);    /* numOfArrays */

  /* Write VPS */
  WRITE_UINT32 (&bs, 0x00, 1);  /* array_completeness */
  WRITE_UINT32 (&bs, 0x00, 1);  /* reserved zero */
  WRITE_UINT32 (&bs, 0x20, 6);  /* Nal_unit_type */
  WRITE_UINT32 (&bs, 0x01, 16); /* numNalus, VPS count = 1 */
  g_assert (GST_BIT_WRITER_BIT_SIZE (&bs) % 8 == 0);
  WRITE_UINT32 (&bs, vps_size, 16);     /* VPS nalUnitLength */
  gst_bit_writer_put_bytes (&bs, vps_info, vps_size);

  /* Write SPS */
  WRITE_UINT32 (&bs, 0x00, 1);  /* array_completeness */
  WRITE_UINT32 (&bs, 0x00, 1);  /* reserved zero */
  WRITE_UINT32 (&bs, 0x21, 6);  /* Nal_unit_type */
  WRITE_UINT32 (&bs, 0x01, 16); /* numNalus, SPS count = 1 */
  g_assert (GST_BIT_WRITER_BIT_SIZE (&bs) % 8 == 0);
  WRITE_UINT32 (&bs, sps_size, 16);     /* SPS nalUnitLength */
  gst_bit_writer_put_bytes (&bs, sps_info, sps_size);

  /* Write PPS */
  WRITE_UINT32 (&bs, 0x00, 1);  /* array_completeness */
  WRITE_UINT32 (&bs, 0x00, 1);  /* reserved zero */
  WRITE_UINT32 (&bs, 0x22, 6);  /* Nal_unit_type */
  WRITE_UINT32 (&bs, 0x01, 16); /* numNalus, PPS count = 1 */
  WRITE_UINT32 (&bs, pps_size, 16);     /* PPS nalUnitLength */
  gst_bit_writer_put_bytes (&bs, pps_info, pps_size);

  buffer = gst_buffer_new_wrapped (
      GST_BIT_WRITER_DATA (&bs), GST_BIT_WRITER_BIT_SIZE (&bs) / 8);
  if (!buffer)
    goto error_alloc_buffer;
  *out_buffer_ptr = buffer;

  gst_bit_writer_clear (&bs, FALSE);

  return GST_MFX_ENCODER_STATUS_SUCCESS;
  /* ERRORS */
bs_error:
  {
    GST_ERROR ("failed to write codec-data");
    gst_bit_writer_clear (&bs, TRUE);
    return FALSE;
  }
error_alloc_buffer:
  {
    GST_ERROR ("failed to allocate codec-data buffer");
    gst_bit_writer_clear (&bs, TRUE);
    return GST_MFX_ENCODER_STATUS_ERROR_ALLOCATION_FAILED;
  }
}

static GstMfxEncoderStatus
gst_mfx_encoder_h265_set_property (GstMfxEncoder * base_encoder,
    gint prop_id, const GValue * value)
{
  switch (prop_id) {
    case GST_MFX_ENCODER_H265_PROP_LA_DEPTH:
      base_encoder->la_depth = g_value_get_uint (value);
      break;
    case GST_MFX_ENCODER_H265_PROP_LOOKAHEAD_DS:
      base_encoder->look_ahead_downsampling = g_value_get_enum (value);
      break;
    default:
      return GST_MFX_ENCODER_STATUS_ERROR_INVALID_PARAMETER;
  }
  return GST_MFX_ENCODER_STATUS_SUCCESS;
}

GST_MFX_ENCODER_DEFINE_CLASS_DATA (H265);

static inline const GstMfxEncoderClass *
gst_mfx_encoder_h265_class (void)
{
  static const GstMfxEncoderClass GstMfxEncoderH265Class = {
    GST_MFX_ENCODER_CLASS_INIT (H265, h265),
    .set_property = gst_mfx_encoder_h265_set_property,
    .get_codec_data = gst_mfx_encoder_h265_get_codec_data
  };
  return &GstMfxEncoderH265Class;
}

GstMfxEncoder *
gst_mfx_encoder_h265_new (GstMfxTaskAggregator * aggregator,
    const GstVideoInfo * info, gboolean mapped)
{
  return gst_mfx_encoder_new (gst_mfx_encoder_h265_class (),
      aggregator, info, mapped);
}

/**
 * gst_mfx_encoder_h265_get_default_properties:
 *
 * Determines the set of common and H.265 specific encoder properties.
 * The caller owns an extra reference to the resulting array of
 * #GstMfxEncoderPropInfo elements, so it shall be released with
 * g_ptr_array_unref () after usage.
 *
 * Return value: the set of encoder properties for #GstMfxEncoderH265,
 *   or %NULL if an error occurred.
 */
GPtrArray *
gst_mfx_encoder_h265_get_default_properties (void)
{
  const GstMfxEncoderClass *const klass = gst_mfx_encoder_h265_class ();
  GPtrArray *props;

  props = gst_mfx_encoder_properties_get_default (klass);
  if (!props)
    return NULL;

 /**
  * GstMfxEncoderH265:la-depth
  *
  * Depth of look ahead in number frames.
  */
  GST_MFX_ENCODER_PROPERTIES_APPEND (props,
      GST_MFX_ENCODER_H265_PROP_LA_DEPTH,
      g_param_spec_uint ("la-depth",
          "Lookahead depth", "Depth of lookahead in frames", 0, 100, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
  * GstMfxEncoderH265:lookahead-ds
  *
  * Look ahead downsampling
  */
  GST_MFX_ENCODER_PROPERTIES_APPEND (props,
      GST_MFX_ENCODER_H265_PROP_LOOKAHEAD_DS,
      g_param_spec_enum ("lookahead-ds",
          "Look ahead downsampling",
          "Look ahead downsampling",
          gst_mfx_encoder_lookahead_ds_get_type (),
          GST_MFX_ENCODER_LOOKAHEAD_DS_AUTO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  return props;
}
