/*
 *  Copyright (C) 2011-2014 Intel Corporation
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

#ifndef GST_MFX_ENCODER_MPEG2_H
#define GST_MFX_ENCODER_MPEG2_H

#include "gstmfxencoder.h"

G_BEGIN_DECLS

#define GST_MFX_ENCODER_MPEG2(encoder) \
  ((GstMfxEncoderMpeg2 *) (encoder))

typedef struct _GstMfxEncoderMpeg2 GstMfxEncoderMpeg2;

GstMfxEncoder *
gst_mfx_encoder_mpeg2_new (GstMfxTaskAggregator * aggregator,
    const GstVideoInfo * info, gboolean mapped);

GPtrArray *
gst_mfx_encoder_mpeg2_get_default_properties(void);

G_END_DECLS

#endif /* GST_MFX_ENCODER_MPEG2_H */
