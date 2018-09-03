/*
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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

#ifndef GST_MFX_ENCODER_PRIV_H
#define GST_MFX_ENCODER_PRIV_H

#include "gstmfxencoder.h"
#include "gstmfxfilter.h"
#include "gstmfxsurfacepool.h"
#include "gstmfxvalue.h"
#include "gstmfxprofile.h"
#include <gst/video/gstvideoutils.h>

G_BEGIN_DECLS

#define GST_MFX_ENCODER_CAST(encoder) \
  ((GstMfxEncoder *)(encoder))

#define GST_MFX_ENCODER_CLASS(klass) \
  ((GstMfxEncoderClass *)(klass))

#define GST_MFX_ENCODER_GET_CLASS(obj) \
  GST_MFX_ENCODER_CLASS(GST_MFX_MINI_OBJECT_GET_CLASS(obj))


/**
 * GST_MFX_ENCODER_VIDEO_INFO:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the #GstVideoInfo of @encoder.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_VIDEO_INFO
#define GST_MFX_ENCODER_VIDEO_INFO(encoder) \
  (&GST_MFX_ENCODER_CAST (encoder)->info)

/**
 * GST_MFX_ENCODER_WIDTH:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the coded width of the picture.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_WIDTH
#define GST_MFX_ENCODER_WIDTH(encoder) \
  (GST_MFX_ENCODER_VIDEO_INFO (encoder)->width)

/**
 * GST_MFX_ENCODER_HEIGHT:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the coded height of the picture.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_HEIGHT
#define GST_MFX_ENCODER_HEIGHT(encoder) \
  (GST_MFX_ENCODER_VIDEO_INFO (encoder)->height)

/**
 * GST_MFX_ENCODER_FPS_N:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the coded framerate numerator.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_FPS_N
#define GST_MFX_ENCODER_FPS_N(encoder) \
  (GST_MFX_ENCODER_VIDEO_INFO (encoder)->fps_n)

/**
 * GST_MFX_ENCODER_FPS_D:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the coded framerate denominator.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_FPS_D
#define GST_MFX_ENCODER_FPS_D(encoder) \
  (GST_MFX_ENCODER_VIDEO_INFO (encoder)->fps_d)

/**
 * GST_MFX_ENCODER_RATE_CONTROL:
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the rate control.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_RATE_CONTROL
#define GST_MFX_ENCODER_RATE_CONTROL(encoder) \
  (GST_MFX_ENCODER_CAST (encoder)->rc_method)

/**
 * GST_MFX_ENCODER_PRESET
 * @encoder: a #GstMfxEncoder
 *
 * Macro that evaluates to the target usage option.
 * This is an internal macro that does not do any run-time type check.
 */
#undef  GST_MFX_ENCODER_PRESET
#define GST_MFX_ENCODER_PRESET(encoder) \
  (GST_MFX_ENCODER_CAST (encoder)->preset)

#define GST_MFX_TYPE_ENCODER_PRESET \
  (gst_mfx_encoder_preset_get_type ())

typedef struct _GstMfxEncoderClass GstMfxEncoderClass;
typedef struct _GstMfxEncoderClassData GstMfxEncoderClassData;

/* Private GstMfxEncoderPropInfo definition */
typedef struct {
  gint prop;
  GParamSpec *pspec;
} GstMfxEncoderPropData;

#define GST_MFX_ENCODER_PROPERTIES_APPEND(props, id, pspec) do {      \
  props = gst_mfx_encoder_properties_append (props, id, pspec);       \
  if (!props)                                                         \
    return NULL;                                                      \
} while (0)

GPtrArray *
gst_mfx_encoder_properties_append(GPtrArray * props, gint prop_id,
  GParamSpec *pspec);

GPtrArray *
gst_mfx_encoder_properties_get_default(const GstMfxEncoderClass * klass);

struct _GstMfxEncoder
{
  /*< private >*/
  GstMfxMiniObject        parent_instance;

  GPtrArray              *properties;
  mfxU16                  profile;

  GstMfxTaskAggregator   *aggregator;
  GstMfxTask             *encode;
  GstMfxFilter           *filter;
  GByteArray             *bitstream;
  gboolean                memtype_is_system;
  gboolean                shared;

  mfxSession              session;
  mfxVideoParam           params;
  mfxFrameInfo            frame_info;
  mfxBitstream            bs;
  mfxU32                  codec;
  gchar                  *plugin_uid;
  GstVideoInfo            info;

  GstClockTime            current_pts;
  GstClockTime            duration;

  /* Encoder params */
  GstMfxEncoderPreset     preset;
  GstMfxRateControl       rc_method;
  guint                   global_quality;
  mfxU16                  bitrate;
  mfxU16                  brc_multiplier;
  mfxU16                  max_buffer_size;
  mfxU16                  vbv_max_bitrate;
  mfxU16                  async_depth;
  mfxU16                  la_depth;
  mfxU16                  gop_size;
  gint                    gop_refdist;
  gint                    idr_interval;
  mfxU16                  num_refs;
  mfxU16                  num_slices;
  mfxU16                  qpi_offset;
  mfxU16                  qpp_offset;
  mfxU16                  qpb_offset;
  mfxU16                  avbr_accuracy;
  mfxU16                  avbr_convergence;
  mfxU16                  jpeg_quality;

  mfxExtCodingOption      extco;
  mfxExtCodingOption2     extco2;
  mfxExtHEVCParam         exthevc;
  mfxExtBuffer           *extparam_internal[3];
  int                     nb_extparam_internal;

  /* H264 specific coding options */
  gboolean                use_cabac;
  gint                    max_slice_size;

  GstMfxOption            mbbrc;
  GstMfxOption            extbrc;
  GstMfxOption            b_strategy;
  GstMfxOption            adaptive_i;
  GstMfxOption            adaptive_b;

  mfxU16                  look_ahead_downsampling;
  mfxU16                  trellis;
};

struct _GstMfxEncoderClassData
{
  /*< private >*/
  GType(*rate_control_get_type)(void);
  GstMfxRateControl default_rate_control;
  guint32 rate_control_mask;
};

#define GST_MFX_ENCODER_DEFINE_CLASS_DATA(CODEC)                      \
  GST_MFX_TYPE_DEFINE_ENUM_SUBSET_FROM_MASK(                          \
  G_PASTE (GstMfxRateControl, CODEC),                                 \
  G_PASTE (gst_mfx_rate_control_, CODEC),                             \
  GST_MFX_TYPE_RATE_CONTROL, SUPPORTED_RATECONTROLS);                 \
  \
  static const GstMfxEncoderClassData g_class_data = {                \
  .rate_control_get_type =                                            \
  G_PASTE (G_PASTE (gst_mfx_rate_control_, CODEC), _get_type),        \
  .default_rate_control = DEFAULT_RATECONTROL,                        \
  .rate_control_mask = SUPPORTED_RATECONTROLS,                        \
}

struct _GstMfxEncoderClass
{
  /*< private >*/
  GstMfxMiniObjectClass parent_class;

  const GstMfxEncoderClassData *class_data;

  gboolean                (*init) (GstMfxEncoder * encoder);
  void                    (*finalize) (GstMfxEncoder * encoder);

  GstMfxEncoderStatus     (*reconfigure) (GstMfxEncoder * encoder);

  GPtrArray *             (*get_default_properties) (void);
  GstMfxEncoderStatus     (*set_property) (GstMfxEncoder * encoder,
    gint prop_id, const GValue * value);

  /* get_codec_data can be NULL */
  GstMfxEncoderStatus     (*get_codec_data) (GstMfxEncoder * encoder,
    GstBuffer ** codec_data);
};

#define GST_MFX_ENCODER_CLASS_HOOK(codec, func) \
  .func = G_PASTE (G_PASTE (G_PASTE (gst_mfx_encoder_,codec),_), func)

#define GST_MFX_ENCODER_CLASS_INIT_BASE(CODEC)                \
  .parent_class = {                                           \
  .size = sizeof (G_PASTE (GstMfxEncoder, CODEC)),            \
  .finalize = (GDestroyNotify) gst_mfx_encoder_finalize       \
}

#define GST_MFX_ENCODER_CLASS_INIT(CODEC, codec)              \
  GST_MFX_ENCODER_CLASS_INIT_BASE (CODEC),                    \
  .class_data = &g_class_data,                                \
  GST_MFX_ENCODER_CLASS_HOOK (codec, init),                   \
  GST_MFX_ENCODER_CLASS_HOOK (codec, finalize),               \
  GST_MFX_ENCODER_CLASS_HOOK (codec, reconfigure),            \
  GST_MFX_ENCODER_CLASS_HOOK (codec, get_default_properties)


GstMfxEncoder *
gst_mfx_encoder_new (const GstMfxEncoderClass * klass,
  	GstMfxTaskAggregator * aggregator, const GstVideoInfo * info, gboolean mapped);

void
gst_mfx_encoder_finalize (GstMfxEncoder * encoder);


G_END_DECLS

#endif /* GST_MFX_ENCODER_PRIV_H */
