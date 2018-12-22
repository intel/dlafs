/*
 * Copyright (C) 2011, Hewlett-Packard Development Company, L.P.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>, Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * Information about the caps fields:
 *
 * header-format:
 *   none: No codec_data and only in-stream headers
 *
 *   asf: codec_data as specified in the ASF specification
 *       Simple/Main profile: 4 byte sequence header without startcode
 *       Advanced profile: Sequence header and entrypoint with startcodes
 *
 *   sequence-layer: codec_data as specified in SMPTE 421M Annex L.2
 *
 *
 * stream-format:
 *   bdu: BDUs with startcodes
 *
 *   bdu-frame: BDUs with startcodes, everything up to and including a frame
 *       per buffer. This also means everything needed to decode a frame, i.e.
 *       field and slice BDUs
 *
 *   sequence-layer-bdu: Sequence layer in first buffer, then BDUs with startcodes
 *
 *   sequence-layer-bdu-frame: Sequence layer in first buffer, then only frame
 *       BDUs with startcodes, i.e. everything up to and including a frame.
 *
 *   sequence-layer-raw-frame: Sequence layer in first buffer, then only frame
 *       BDUs without startcodes. Only for simple/main profile.
 *
 *   sequence-layer-frame-layer: As specified in SMPTE 421M Annex L, sequence-layer
 *       first, then BDUs inside frame-layer
 *
 *   asf: As specified in the ASF specification.
 *       For simple/main profile a single frame BDU without startcodes per buffer
 *       For advanced profile one or many BDUs with/without startcodes:
 *           Startcodes required if non-frame BDU or multiple BDUs per buffer
 *           unless frame BDU followed by field BDU. In that case only second (field)
 *           startcode required.
 *
 *   frame-layer: As specified in SMPTE 421M Annex L.2
 *
 *
 * If no stream-format is given in the caps we do the following:
 *
 *   0) If header-format=asf we assume stream-format=asf
 *   1) If first buffer starts with sequence header startcode
 *      we assume stream-format=bdu (or bdu-frame, doesn't matter
 *      for the input because we're parsing anyway)
 *   2) If first buffer starts with sequence layer startcode
 *      1) If followed by sequence header or frame startcode
 *         we assume stream-format=sequence-layer-bdu (or -bdu-frame,
 *         doesn't matter for the input because we're parsing anyway)
 *      2) Otherwise we assume stream-format=sequence-layer-frame-layer
 *   3) Otherwise
 *      1) If header-format=sequence-layer we assume stream-format=frame-layer
 *      2) If header-format=none we error out
 */

#include "gstvc1parse.h"

#include <gst/base/base.h>
#include <gst/pbutils/pbutils.h>
#include <string.h>

GST_DEBUG_CATEGORY (vc1_parse_debug);
#define GST_CAT_DEFAULT vc1_parse_debug

static const struct
{
  gchar str[15];
  VC1HeaderFormat en;
} header_formats[] = {
  {
  "none", VC1_HEADER_FORMAT_NONE}, {
  "asf", VC1_HEADER_FORMAT_ASF}, {
  "sequence-layer", VC1_HEADER_FORMAT_SEQUENCE_LAYER}
};

static const struct
{
  gchar str[27];
  VC1StreamFormat en;
} stream_formats[] = {
  {
  "bdu", VC1_STREAM_FORMAT_BDU}, {
  "bdu-frame", VC1_STREAM_FORMAT_BDU_FRAME}, {
  "sequence-layer-bdu", VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU}, {
  "sequence-layer-bdu-frame", VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME}, {
  "sequence-layer-raw-frame", VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME}, {
  "sequence-layer-frame-layer", VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER}, {
  "asf", VC1_STREAM_FORMAT_ASF}, {
  "frame-layer", VC1_STREAM_FORMAT_FRAME_LAYER}
};

static const struct
{
  gchar str[5];
  GstVC1ParseFormat en;
} parse_formats[] = {
  {
  "WMV3", GST_VC1_PARSE_FORMAT_WMV3}, {
  "WVC1", GST_VC1_PARSE_FORMAT_WVC1}
};

static const gchar *
stream_format_to_string (VC1StreamFormat stream_format)
{
  return stream_formats[stream_format].str;
}

static VC1StreamFormat
stream_format_from_string (const gchar * stream_format)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (stream_formats); i++) {
    if (strcmp (stream_formats[i].str, stream_format) == 0)
      return stream_formats[i].en;
  }
  return -1;
}

static const gchar *
header_format_to_string (VC1HeaderFormat header_format)
{
  return header_formats[header_format].str;
}

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-wmv, wmvversion=(int) 3, "
        "format=(string) {WVC1, WMV3}"));

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-wmv, wmvversion=(int) 3, "
        "format=(string) {WVC1, WMV3}"));


#define parent_class gst_mfx_vc1_parse_parent_class
G_DEFINE_TYPE (GstMfxVC1Parse, gst_mfx_vc1_parse, GST_TYPE_BASE_PARSE);

static void gst_vc1_parse_finalize (GObject * object);

static gboolean gst_vc1_parse_start (GstBaseParse * parse);
static gboolean gst_vc1_parse_stop (GstBaseParse * parse);
static GstFlowReturn gst_vc1_parse_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);
static GstFlowReturn gst_vc1_parse_pre_push_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);
static gboolean gst_vc1_parse_set_caps (GstBaseParse * parse, GstCaps * caps);
static GstCaps *gst_vc1_parse_get_sink_caps (GstBaseParse * parse,
    GstCaps * filter);
static GstFlowReturn gst_vc1_parse_detect (GstBaseParse * parse,
    GstBuffer * buffer);

static void gst_vc1_parse_reset (GstMfxVC1Parse * vc1parse);
static gboolean gst_vc1_parse_handle_seq_layer (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size);
static gboolean gst_vc1_parse_handle_seq_hdr (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size);
static gboolean gst_vc1_parse_handle_entrypoint (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size);
static void gst_vc1_parse_update_stream_format_properties (GstMfxVC1Parse *
    vc1parse);

static void
gst_mfx_vc1_parse_class_init (GstMfxVC1ParseClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (vc1_parse_debug, "vc1parse", 0, "vc1 parser");

  gobject_class->finalize = gst_vc1_parse_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_static_metadata (element_class, "VC1 parser",
      "Codec/Parser/Converter/Video",
      "Parses VC1 streams",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  parse_class->start = GST_DEBUG_FUNCPTR (gst_vc1_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_vc1_parse_stop);
  parse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_vc1_parse_handle_frame);
  parse_class->pre_push_frame =
      GST_DEBUG_FUNCPTR (gst_vc1_parse_pre_push_frame);
  parse_class->set_sink_caps = GST_DEBUG_FUNCPTR (gst_vc1_parse_set_caps);
  parse_class->get_sink_caps = GST_DEBUG_FUNCPTR (gst_vc1_parse_get_sink_caps);
  parse_class->detect = GST_DEBUG_FUNCPTR (gst_vc1_parse_detect);
}

static void
gst_mfx_vc1_parse_init (GstMfxVC1Parse * vc1parse)
{
  /* Default values for stream-format=raw, i.e.
   * raw VC1 frames with startcodes */
  gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), TRUE);
  gst_base_parse_set_has_timing_info (GST_BASE_PARSE (vc1parse), FALSE);

  gst_vc1_parse_reset (vc1parse);
  GST_PAD_SET_ACCEPT_INTERSECT (GST_BASE_PARSE_SINK_PAD (vc1parse));
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_BASE_PARSE_SINK_PAD (vc1parse));
}

static void
gst_vc1_parse_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_vc1_parse_reset (GstMfxVC1Parse * vc1parse)
{
  vc1parse->profile = -1;
  vc1parse->level = -1;
  vc1parse->format = 0;
  vc1parse->width = 0;
  vc1parse->height = 0;
  vc1parse->fps_n = vc1parse->fps_d = 0;
  vc1parse->frame_duration = GST_CLOCK_TIME_NONE;
  vc1parse->fps_from_caps = FALSE;
  vc1parse->par_n = vc1parse->par_d = 0;
  vc1parse->par_from_caps = FALSE;

  vc1parse->renegotiate = TRUE;
  vc1parse->update_caps = TRUE;
  vc1parse->sent_codec_tag = FALSE;

  vc1parse->input_header_format = VC1_HEADER_FORMAT_NONE;
  vc1parse->input_stream_format = VC1_STREAM_FORMAT_BDU;
  vc1parse->output_stream_format = VC1_STREAM_FORMAT_BDU;
  gst_buffer_replace (&vc1parse->seq_layer_buffer, NULL);
  gst_buffer_replace (&vc1parse->seq_hdr_buffer, NULL);
  gst_buffer_replace (&vc1parse->entrypoint_buffer, NULL);

  vc1parse->codec_data_sent = FALSE;
}

static gboolean
gst_vc1_parse_start (GstBaseParse * parse)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);

  GST_DEBUG_OBJECT (parse, "start");
  gst_vc1_parse_reset (vc1parse);

  vc1parse->detecting_stream_format = TRUE;

  return TRUE;
}

static gboolean
gst_vc1_parse_stop (GstBaseParse * parse)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);

  GST_DEBUG_OBJECT (parse, "stop");
  gst_vc1_parse_reset (vc1parse);

  return TRUE;
}

static gboolean
gst_vc1_parse_is_format_allowed (GstMfxVC1Parse * vc1parse)
{
  if (vc1parse->output_stream_format == vc1parse->input_stream_format)
    return TRUE;

  GST_DEBUG_OBJECT (vc1parse, "check stream-format conversion");
  switch (vc1parse->output_stream_format) {
    case VC1_STREAM_FORMAT_BDU:
      if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU_FRAME ||
          vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME ||
          vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME ||
          vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER ||
          vc1parse->input_stream_format == VC1_STREAM_FORMAT_FRAME_LAYER)
        goto conversion_not_supported;
      break;

    case VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER:
      if (vc1parse->input_stream_format != VC1_STREAM_FORMAT_FRAME_LAYER &&
          vc1parse->input_stream_format != VC1_STREAM_FORMAT_ASF)
        goto conversion_not_supported;
      break;

    default:
      g_assert_not_reached ();
      break;
  }

  return TRUE;

conversion_not_supported:
  GST_ERROR_OBJECT (vc1parse, "stream conversion not implemented yet");
  return FALSE;
}

static gboolean
gst_vc1_parse_renegotiate (GstMfxVC1Parse * vc1parse)
{
  /* Negotiate with downstream here */
  GST_DEBUG_OBJECT (vc1parse, "Renegotiating");

  if (vc1parse->profile == GST_VC1_PROFILE_ADVANCED)
    vc1parse->output_stream_format = VC1_STREAM_FORMAT_BDU;
  else
    vc1parse->output_stream_format = VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER;

  if (!gst_vc1_parse_is_format_allowed (vc1parse))
    return FALSE;

  vc1parse->renegotiate = FALSE;
  vc1parse->update_caps = TRUE;

  GST_INFO_OBJECT (vc1parse, "input %s/%s, negotiated %s with downstream",
      header_format_to_string (vc1parse->input_header_format),
      stream_format_to_string (vc1parse->input_stream_format),
      stream_format_to_string (vc1parse->output_stream_format));

  return TRUE;
}

static GstCaps *
gst_vc1_parse_get_sink_caps (GstBaseParse * parse, GstCaps * filter)
{
  GstCaps *peercaps;
  GstCaps *templ;
  GstCaps *ret;

  templ = gst_pad_get_pad_template_caps (GST_BASE_PARSE_SINK_PAD (parse));
  if (filter) {
    GstCaps *fcopy = gst_caps_copy (filter);
    /* Remove the fields we convert */
    peercaps = gst_pad_peer_query_caps (GST_BASE_PARSE_SRC_PAD (parse), fcopy);
    gst_caps_unref (fcopy);
  } else
    peercaps = gst_pad_peer_query_caps (GST_BASE_PARSE_SRC_PAD (parse), NULL);

  if (peercaps) {
    /* Remove the stream-format and header-format fields
     * and add the generic ones again by intersecting
     * with our template */
    peercaps = gst_caps_make_writable (peercaps);

    ret = gst_caps_intersect_full (peercaps, templ, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (peercaps);
    gst_caps_unref (templ);
  } else {
    ret = templ;
  }

  if (filter) {
    GstCaps *tmp =
        gst_caps_intersect_full (filter, ret, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (ret);
    ret = tmp;
  }

  return ret;
}

static GstFlowReturn
gst_vc1_parse_detect (GstBaseParse * parse, GstBuffer * buffer)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);
  GstMapInfo minfo;
  guint8 *data;
  gint size;

  if (!vc1parse->detecting_stream_format)
    return GST_FLOW_OK;

  if (!gst_buffer_map (buffer, &minfo, GST_MAP_READ))
    return GST_FLOW_ERROR;

  data = minfo.data;
  size = minfo.size;

#if 0
  /* FIXME: disable BDU check for now as BDU parsing needs more work.
   */
  while (size >= 4) {
    guint32 startcode = GST_READ_UINT32_BE (data);

    if ((startcode & 0xffffff00) == 0x00000100) {
      GST_DEBUG_OBJECT (vc1parse, "Found BDU startcode");
      vc1parse->input_stream_format = VC1_STREAM_FORMAT_BDU_FRAME;
      goto detected;
    }

    data += 4;
    size -= 4;
  }
#endif

  while (size >= 40) {
    if (data[3] == 0xc5 && GST_READ_UINT32_LE (data + 4) == 0x00000004 &&
        GST_READ_UINT32_LE (data + 20) == 0x0000000c) {
      guint32 startcode;

      GST_DEBUG_OBJECT (vc1parse, "Found sequence layer");
      startcode = GST_READ_UINT32_BE (data + 36);
      if ((startcode & 0xffffff00) == 0x00000100) {
        GST_DEBUG_OBJECT (vc1parse, "Found BDU startcode after sequence layer");
        vc1parse->input_stream_format =
            VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME;
        goto detected;
      } else {
        GST_DEBUG_OBJECT (vc1parse,
            "Assuming sequence-layer-frame-layer stream format");
        vc1parse->input_stream_format =
            VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER;
        goto detected;
      }
    }
    data += 4;
    size -= 4;
  }

  if (gst_buffer_get_size (buffer) <= 128) {
    GST_DEBUG_OBJECT (vc1parse, "Requesting more data");
    gst_buffer_unmap (buffer, &minfo);
    return GST_FLOW_NOT_NEGOTIATED;
  }

  if (GST_BASE_PARSE_DRAINING (vc1parse)) {
    GST_ERROR_OBJECT (vc1parse, "Failed to detect or assume a stream format "
        "and draining now");
    gst_buffer_unmap (buffer, &minfo);
    return GST_FLOW_ERROR;
  }

  /* Otherwise we try some heuristics */
  if (vc1parse->input_header_format == VC1_HEADER_FORMAT_ASF) {
    GST_DEBUG_OBJECT (vc1parse, "Assuming ASF stream format");
    vc1parse->input_stream_format = VC1_STREAM_FORMAT_ASF;
    goto detected;
  } else if (vc1parse->input_header_format == VC1_HEADER_FORMAT_SEQUENCE_LAYER) {
    GST_DEBUG_OBJECT (vc1parse, "Assuming frame-layer stream format");
    vc1parse->input_stream_format = VC1_STREAM_FORMAT_FRAME_LAYER;
    goto detected;
  } else {
    GST_ERROR_OBJECT (vc1parse, "Can't detect or assume a stream format");
    gst_buffer_unmap (buffer, &minfo);
    return GST_FLOW_ERROR;
  }

  g_assert_not_reached ();
  return GST_FLOW_ERROR;

detected:

  gst_buffer_unmap (buffer, &minfo);
  vc1parse->detecting_stream_format = FALSE;
  gst_vc1_parse_update_stream_format_properties (vc1parse);
  return GST_FLOW_OK;
}

static int
gst_vc1_parse_get_max_framerate (GstMfxVC1Parse * vc1parse)
{
  /* http://wiki.multimedia.cx/index.php?title=VC-1#Setup_Data_.2F_Sequence_Layer */
  switch (vc1parse->profile) {
    case GST_VC1_PROFILE_SIMPLE:
      switch (vc1parse->level) {
        case GST_VC1_LEVEL_LOW:
          return 15;
        case GST_VC1_LEVEL_MEDIUM:
          return 30;
        default:
          g_assert_not_reached ();
          return 0;
      }
      break;
    case GST_VC1_PROFILE_MAIN:
      switch (vc1parse->level) {
        case GST_VC1_LEVEL_LOW:
          return 24;
        case GST_VC1_LEVEL_MEDIUM:
          return 30;
        case GST_VC1_LEVEL_HIGH:
          return 30;
        default:
          g_assert_not_reached ();
          return 0;
      }
      break;
    case GST_VC1_PROFILE_ADVANCED:
      switch (vc1parse->level) {
        case GST_VC1_LEVEL_L0:
          return 30;
        case GST_VC1_LEVEL_L1:
          return 30;
        case GST_VC1_LEVEL_L2:
          return 60;
        case GST_VC1_LEVEL_L3:
          return 60;
        case GST_VC1_LEVEL_L4:
          return 60;
        default:
          g_assert_not_reached ();
          return 0;
      }
      break;
    default:
      g_assert_not_reached ();
      return 0;
  }
}

static GstBuffer *
gst_vc1_parse_make_sequence_layer (GstMfxVC1Parse * vc1parse)
{
  GstBuffer *seq_layer_buffer;
  guint8 *data;
  guint32 structC = 0;
  GstMapInfo minfo;

  seq_layer_buffer = gst_buffer_new_and_alloc (36);
  gst_buffer_map (seq_layer_buffer, &minfo, GST_MAP_WRITE);

  data = minfo.data;
  /* According to SMPTE 421M Annex L, the sequence layer shall be
   * represented as a sequence of 32 bit unsigned integers and each
   * integers should be serialized in little-endian byte-order except for
   * STRUCT_C which should be serialized in big-endian byte-order. */

  /* Unknown number of frames and start code */
  data[0] = 0xff;
  data[1] = 0xff;
  data[2] = 0xff;
  data[3] = 0xc5;

  /* 0x00000004 */
  GST_WRITE_UINT32_LE (data + 4, 4);

  /* structC */
  structC |= (vc1parse->profile << 30);
  if (vc1parse->profile != GST_VC1_PROFILE_ADVANCED) {
    /* Build simple/main structC from sequence header */
    structC |= (vc1parse->seq_hdr.struct_c.wmvp << 28);
    structC |= (vc1parse->seq_hdr.struct_c.frmrtq_postproc << 25);
    structC |= (vc1parse->seq_hdr.struct_c.bitrtq_postproc << 20);
    structC |= (vc1parse->seq_hdr.struct_c.loop_filter << 19);
    /* Reserved3 shall be set to zero */
    structC |= (vc1parse->seq_hdr.struct_c.multires << 17);
    /* Reserved4 shall be set to one */
    structC |= (1 << 16);
    structC |= (vc1parse->seq_hdr.struct_c.fastuvmc << 15);
    structC |= (vc1parse->seq_hdr.struct_c.extended_mv << 14);
    structC |= (vc1parse->seq_hdr.struct_c.dquant << 12);
    structC |= (vc1parse->seq_hdr.struct_c.vstransform << 11);
    /* Reserved5 shall be set to zero */
    structC |= (vc1parse->seq_hdr.struct_c.overlap << 9);
    structC |= (vc1parse->seq_hdr.struct_c.syncmarker << 8);
    structC |= (vc1parse->seq_hdr.struct_c.rangered << 7);
    structC |= (vc1parse->seq_hdr.struct_c.maxbframes << 4);
    structC |= (vc1parse->seq_hdr.struct_c.quantizer << 2);
    structC |= (vc1parse->seq_hdr.struct_c.finterpflag << 1);
    /* Reserved6 shall be set to one */
    structC |= 1;
  }
  GST_WRITE_UINT32_BE (data + 8, structC);

  /* structA */
  if (vc1parse->profile != GST_VC1_PROFILE_ADVANCED) {
    GST_WRITE_UINT32_LE (data + 12, vc1parse->height);
    GST_WRITE_UINT32_LE (data + 16, vc1parse->width);
  } else {
    GST_WRITE_UINT32_LE (data + 12, 0);
    GST_WRITE_UINT32_LE (data + 16, 0);
  }

  /* 0x0000000c */
  GST_WRITE_UINT32_LE (data + 20, 0x0000000c);

  /* structB */
  /* Unknown HRD_BUFFER */
  GST_WRITE_UINT24_LE (data + 24, 0);
  if ((gint) vc1parse->level != -1)
    data[27] = (vc1parse->level << 5);
  else
    data[27] = (0x4 << 5);      /* Use HIGH level */
  /* Unknown HRD_RATE */
  GST_WRITE_UINT32_LE (data + 28, 0);
  /* Framerate */
  if (vc1parse->fps_d == 0) {
    /* If not known, it seems we need to put in the maximum framerate
       possible for the profile/level used (this is for RTP
       (https://tools.ietf.org/html/draft-ietf-avt-rtp-vc1-06#section-6.1),
       so likely elsewhere too */
    GST_WRITE_UINT32_LE (data + 32, gst_vc1_parse_get_max_framerate (vc1parse));
  } else {
    GST_WRITE_UINT32_LE (data + 32,
        ((guint32) (((gdouble) vc1parse->fps_n) /
                ((gdouble) vc1parse->fps_d) + 0.5)));
  }

  gst_buffer_unmap (seq_layer_buffer, &minfo);

  return seq_layer_buffer;
}

static gboolean
gst_vc1_parse_update_caps (GstMfxVC1Parse * vc1parse)
{
  GstCaps *caps;
  GstVC1Profile profile = -1;
  const gchar *stream_format;

  if (gst_pad_has_current_caps (GST_BASE_PARSE_SRC_PAD (vc1parse))
      && !vc1parse->update_caps)
    return TRUE;

  caps = gst_caps_new_simple ("video/x-wmv", "wmvversion", G_TYPE_INT, 3, NULL);

  stream_format = stream_format_to_string (vc1parse->output_stream_format);
  gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, stream_format, NULL);

  /* Must have this here from somewhere */
  g_assert (vc1parse->width != 0 && vc1parse->height != 0);
  gst_caps_set_simple (caps, "width", G_TYPE_INT, vc1parse->width, "height",
      G_TYPE_INT, vc1parse->height, NULL);
  if (vc1parse->fps_d != 0) {
    gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION, vc1parse->fps_n,
        vc1parse->fps_d, NULL);

    vc1parse->frame_duration = gst_util_uint64_scale (GST_SECOND,
        vc1parse->fps_d, vc1parse->fps_n);
  }

  if (vc1parse->par_n != 0 && vc1parse->par_d != 0)
    gst_caps_set_simple (caps, "pixel-aspect-ratio", GST_TYPE_FRACTION,
        vc1parse->par_n, vc1parse->par_d, NULL);

  if (vc1parse->seq_hdr_buffer)
    profile = vc1parse->seq_hdr.profile;
  else if (vc1parse->seq_layer_buffer)
    profile = vc1parse->seq_layer.struct_c.profile;
  else
    g_assert_not_reached ();

  if (profile == GST_VC1_PROFILE_ADVANCED) {
    const gchar *level = NULL;
    /* Caller must make sure this is valid here */
    g_assert (vc1parse->seq_hdr_buffer);
    switch ((GstVC1Level) vc1parse->seq_hdr.advanced.level) {
      case GST_VC1_LEVEL_L0:
        level = "0";
        break;
      case GST_VC1_LEVEL_L1:
        level = "1";
        break;
      case GST_VC1_LEVEL_L2:
        level = "2";
        break;
      case GST_VC1_LEVEL_L3:
        level = "3";
        break;
      case GST_VC1_LEVEL_L4:
        level = "4";
        break;
      default:
        g_assert_not_reached ();
        break;
    }

    gst_caps_set_simple (caps, "format", G_TYPE_STRING, "WVC1",
        "profile", G_TYPE_STRING, "advanced",
        "level", G_TYPE_STRING, level, NULL);
  } else if (profile == GST_VC1_PROFILE_SIMPLE
      || profile == GST_VC1_PROFILE_MAIN) {
    const gchar *profile_str;

    if (profile == GST_VC1_PROFILE_SIMPLE)
      profile_str = "simple";
    else
      profile_str = "main";

    gst_caps_set_simple (caps, "format", G_TYPE_STRING, "WMV3",
        "profile", G_TYPE_STRING, profile_str, NULL);

    if (vc1parse->seq_layer_buffer) {
      const gchar *level = NULL;
      switch (vc1parse->seq_layer.struct_b.level) {
        case GST_VC1_LEVEL_LOW:
          level = "low";
          break;
        case GST_VC1_LEVEL_MEDIUM:
          level = "medium";
          break;
        case GST_VC1_LEVEL_HIGH:
          level = "high";
          break;
        default:
          g_assert_not_reached ();
          break;
      }

      gst_caps_set_simple (caps, "level", G_TYPE_STRING, level, NULL);
    }
  } else {
    g_assert_not_reached ();
  }

  GST_DEBUG_OBJECT (vc1parse, "Setting caps %" GST_PTR_FORMAT, caps);
  gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (vc1parse), caps);
  gst_caps_unref (caps);
  vc1parse->update_caps = FALSE;
  return TRUE;
}

static inline void
calculate_mb_size (GstVC1SeqHdr * seqhdr, guint width, guint height)
{
  seqhdr->mb_width = (width + 15) >> 4;
  seqhdr->mb_height = (height + 15) >> 4;
  seqhdr->mb_stride = seqhdr->mb_width + 1;
}

static gboolean
gst_vc1_parse_handle_bdu (GstMfxVC1Parse * vc1parse, GstVC1StartCode startcode,
    GstBuffer * buffer, guint offset, guint size)
{
  GST_DEBUG_OBJECT (vc1parse, "Handling BDU with startcode 0x%02x", startcode);

  switch (startcode) {
    case GST_VC1_SEQUENCE:{
      GST_DEBUG_OBJECT (vc1parse, "Have new SequenceHeader header");
      if (!gst_vc1_parse_handle_seq_hdr (vc1parse, buffer, offset, size)) {
        GST_ERROR_OBJECT (vc1parse, "Invalid VC1 sequence header");
        return FALSE;
      }
      break;
    }
    case GST_VC1_ENTRYPOINT:
      GST_DEBUG_OBJECT (vc1parse, "Have new EntryPoint header");
      if (!gst_vc1_parse_handle_entrypoint (vc1parse, buffer, offset, size)) {
        GST_ERROR_OBJECT (vc1parse, "Invalid VC1 entrypoint");
        return FALSE;
      }
      break;
    case GST_VC1_FRAME:
      /* TODO: Check if keyframe */
      break;
    default:
      break;
  }

  return TRUE;
}

static gboolean
gst_vc1_parse_handle_bdus (GstMfxVC1Parse * vc1parse, GstBuffer * buffer,
    guint offset, guint size)
{
  GstVC1BDU bdu;
  GstVC1ParserResult pres;
  guint8 *data;
  GstMapInfo minfo;

  gst_buffer_map (buffer, &minfo, GST_MAP_READ);

  data = minfo.data + offset;

  do {
    memset (&bdu, 0, sizeof (bdu));
    pres = gst_vc1_identify_next_bdu (data, size, &bdu);
    if (pres == GST_VC1_PARSER_OK || pres == GST_VC1_PARSER_NO_BDU_END) {
      if (pres == GST_VC1_PARSER_NO_BDU_END) {
        pres = GST_VC1_PARSER_OK;
        bdu.size = size - bdu.offset;
      }

      data += bdu.offset;
      size -= bdu.offset;

      if (!gst_vc1_parse_handle_bdu (vc1parse, bdu.type, buffer,
              data - minfo.data, bdu.size)) {
        gst_buffer_unmap (buffer, &minfo);
        return FALSE;
      }

      data += bdu.size;
      size -= bdu.size;
    }
  } while (pres == GST_VC1_PARSER_OK && size > 0);

  gst_buffer_unmap (buffer, &minfo);

  if (pres != GST_VC1_PARSER_OK) {
    GST_DEBUG_OBJECT (vc1parse, "Failed to parse BDUs");
    return FALSE;
  }
  return TRUE;
}

static GstFlowReturn
gst_vc1_parse_handle_frame (GstBaseParse * parse, GstBaseParseFrame * frame,
    gint * skipsize)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);
  GstBuffer *buffer = frame->buffer;
  guint8 *data;
  gsize size;
  gsize framesize = -1;
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo minfo;

  memset (&minfo, 0, sizeof (minfo));

  *skipsize = 0;

  if (vc1parse->renegotiate
      || gst_pad_check_reconfigure (GST_BASE_PARSE_SRC_PAD (parse))) {
    if (!gst_vc1_parse_renegotiate (vc1parse)) {
      GST_ERROR_OBJECT (vc1parse, "Failed to negotiate with downstream");
      ret = GST_FLOW_NOT_NEGOTIATED;
      goto done;
    }
  }

  if (!gst_buffer_map (buffer, &minfo, GST_MAP_READ)) {
    GST_ERROR_OBJECT (vc1parse, "Failed to map buffer");
    ret = GST_FLOW_ERROR;
    goto done;
  }

  data = minfo.data;
  size = minfo.size;

  /* First check if we have a valid, complete frame here */
  if (!vc1parse->seq_layer_buffer
      && (vc1parse->input_stream_format == VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER)) {
    if (data[3] == 0xc5 && GST_READ_UINT32_LE (data + 4) == 0x00000004
        && GST_READ_UINT32_LE (data + 20) == 0x0000000c) {
      framesize = 36;
    } else {
      *skipsize = 1;
    }
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU ||
      vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU_FRAME ||
      (vc1parse->seq_layer_buffer
          && (vc1parse->input_stream_format ==
              VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
              || vc1parse->input_stream_format ==
              VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME))) {
    GstVC1ParserResult pres;
    GstVC1BDU bdu;

    g_assert (size >= 4);
    memset (&bdu, 0, sizeof (bdu));
    GST_DEBUG_OBJECT (vc1parse,
        "Handling buffer of size %" G_GSIZE_FORMAT " at offset %"
        G_GUINT64_FORMAT, size, GST_BUFFER_OFFSET (buffer));
    /* XXX: when a buffer contains multiple BDUs, does the first one start with
     * a startcode?
     */
    pres = gst_vc1_identify_next_bdu (data, size, &bdu);
    switch (pres) {
      case GST_VC1_PARSER_OK:
        GST_DEBUG_OBJECT (vc1parse, "Have complete BDU");
        if (bdu.sc_offset > 4) {
          *skipsize = bdu.sc_offset;
        } else {
          framesize = bdu.offset + bdu.size;
        }
        break;
      case GST_VC1_PARSER_BROKEN_DATA:
        GST_ERROR_OBJECT (vc1parse, "Broken data");
        *skipsize = 1;
        break;
      case GST_VC1_PARSER_NO_BDU:
        GST_DEBUG_OBJECT (vc1parse, "Found no BDU startcode");
        *skipsize = size - 3;
        break;
      case GST_VC1_PARSER_NO_BDU_END:
        GST_DEBUG_OBJECT (vc1parse, "Found no BDU end");
        if (G_UNLIKELY (GST_BASE_PARSE_DRAINING (vc1parse))) {
          GST_DEBUG_OBJECT (vc1parse, "Draining - assuming complete frame");
          framesize = size;
        } else {
          /* Need more data */
          *skipsize = 0;
        }
        break;
      case GST_VC1_PARSER_ERROR:
        GST_ERROR_OBJECT (vc1parse, "Parsing error");
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_ASF ||
      (vc1parse->seq_layer_buffer
          && vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME)) {
    /* Must be packetized already */
    framesize = size;
  } else {
    /* frame-layer or sequence-layer-frame-layer */
    g_assert (size >= 8);
    /* Parse frame layer size */
    framesize = GST_READ_UINT24_LE (data) + 8;
  }


  if (framesize == -1) {
    GST_DEBUG_OBJECT (vc1parse, "Not a complete frame, skipping %d", *skipsize);
    ret = GST_FLOW_OK;
    goto done;
  }
  g_assert (*skipsize == 0);

  /* We have a complete frame at this point */

  if (!vc1parse->seq_layer_buffer
      && (vc1parse->input_stream_format == VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME
          || vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER)) {
    g_assert (size >= 36);
    if (!gst_vc1_parse_handle_seq_layer (vc1parse, buffer, 0, size)) {
      GST_ERROR_OBJECT (vc1parse, "Invalid sequence layer");
      ret = GST_FLOW_ERROR;
      goto done;
    }

    frame->flags |= GST_BASE_PARSE_FRAME_FLAG_NO_FRAME;

    if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
        || vc1parse->input_stream_format ==
        VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME) {
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 4);
    } else if (vc1parse->input_stream_format ==
        VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME) {
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 1);
    } else {
      /* frame-layer */
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 8);
    }
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU ||
      vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU_FRAME ||
      (vc1parse->seq_layer_buffer
          && (vc1parse->input_stream_format ==
              VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
              || vc1parse->input_stream_format ==
              VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME))) {
    GstVC1StartCode startcode;

    /* Is already a complete BDU, should have at least the startcode */
    g_assert (size >= 4);
    startcode = data[3];

    if (startcode != GST_VC1_SEQUENCE) {
      if (!vc1parse->seq_hdr_buffer && !vc1parse->seq_layer_buffer) {
        GST_ERROR_OBJECT (vc1parse,
            "Need sequence header/layer before anything else");
        ret = GST_FLOW_ERROR;
        goto done;
      }
    } else if (startcode != GST_VC1_ENTRYPOINT
        && vc1parse->profile == GST_VC1_PROFILE_ADVANCED) {
      if (vc1parse->seq_hdr_buffer && !vc1parse->entrypoint_buffer) {
        GST_ERROR_OBJECT (vc1parse,
            "Need entrypoint header after the sequence header for the "
            "advanced profile");
        ret = GST_FLOW_ERROR;
        goto done;
      }
    }

    if (!gst_vc1_parse_handle_bdu (vc1parse, startcode, buffer, 4, size - 4)) {
      ret = GST_FLOW_ERROR;
      goto done;
    }
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_ASF ||
      (vc1parse->seq_layer_buffer
          && vc1parse->input_stream_format ==
          VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME)) {
    GST_LOG_OBJECT (vc1parse, "Have new ASF or RAW data unit");

    if (!vc1parse->seq_hdr_buffer && !vc1parse->seq_layer_buffer) {
      GST_ERROR_OBJECT (vc1parse, "Need a sequence header or sequence layer");
      ret = GST_FLOW_ERROR;
      goto done;
    }

    if (GST_CLOCK_TIME_IS_VALID (vc1parse->frame_duration))
      GST_BUFFER_DURATION (buffer) = vc1parse->frame_duration;

    /* Might be multiple BDUs here, complex... */
    if (vc1parse->profile == GST_VC1_PROFILE_ADVANCED) {
      gboolean startcodes = FALSE;

      if (size >= 4) {
        guint32 startcode = GST_READ_UINT32_BE (data + 4);

        startcodes = ((startcode & 0xffffff00) == 0x00000100);
      }

      if (startcodes) {
        if (!gst_vc1_parse_handle_bdus (vc1parse, buffer, 0, size)) {
          ret = GST_FLOW_ERROR;
          goto done;
        }

        /* For the advanced profile we need a sequence header here */
        if (!vc1parse->seq_hdr_buffer) {
          GST_ERROR_OBJECT (vc1parse, "Need sequence header");
          ret = GST_FLOW_ERROR;
          goto done;
        }
      } else {
        /* Must be a frame or a frame + field */
        /* TODO: Check if keyframe */
      }
    } else {
      /* In simple/main, we basically have a raw frame, so parse it */
      GstVC1ParserResult pres;
      GstVC1FrameHdr frame_hdr;
      GstVC1SeqHdr seq_hdr;

      if (!vc1parse->seq_hdr_buffer) {
        /* Build seq_hdr from sequence-layer to be able to parse frame */
        seq_hdr.profile = vc1parse->profile;
        seq_hdr.struct_c = vc1parse->seq_layer.struct_c;
        calculate_mb_size (&seq_hdr, vc1parse->seq_layer.struct_a.horiz_size,
            vc1parse->seq_layer.struct_a.vert_size);
      } else {
        seq_hdr = vc1parse->seq_hdr;
      }

      pres = gst_vc1_parse_frame_header (data, size, &frame_hdr,
          &seq_hdr, NULL);
      if (pres != GST_VC1_PARSER_OK) {
        GST_ERROR_OBJECT (vc1parse, "Invalid VC1 frame header");
        ret = GST_FLOW_ERROR;
        goto done;
      }

      if (frame_hdr.ptype == GST_VC1_PICTURE_TYPE_I)
        GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
      else
        GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }
  } else {
    GstVC1ParserResult pres;
    GstVC1FrameLayer flayer;
    gboolean startcodes = FALSE;

    /* frame-layer or sequence-layer-frame-layer */

    /* Check if the frame-layer data contains BDUs with startcodes.
     * Startcodes are not allowed in raw WMV9/VC1 streams
     */
    if (size >= 8 + 4) {
      guint32 startcode = GST_READ_UINT32_BE (data + 8);

      startcodes = ((startcode & 0xffffff00) == 0x00000100);
    }

    /* We either need a sequence layer or sequence header here
     * or this has to be an advanced profile stream.
     *
     * For the advanced profile the frame-layer data contains
     * BDUs with startcodes and includes the sequence header
     */
    if (!vc1parse->seq_layer_buffer && !vc1parse->seq_hdr_buffer && !startcodes) {
      GST_ERROR_OBJECT (vc1parse, "Need a sequence header or sequence layer");
      ret = GST_FLOW_ERROR;
      goto done;
    }

    if ((vc1parse->seq_layer_buffer || vc1parse->seq_hdr_buffer)
        && vc1parse->profile == GST_VC1_PROFILE_ADVANCED && !startcodes) {
      GST_ERROR_OBJECT (vc1parse,
          "Advanced profile frame-layer data must start with startcodes");
      ret = GST_FLOW_ERROR;
      goto done;
    }

    memset (&flayer, 0, sizeof (flayer));

    pres = gst_vc1_parse_frame_layer (data, size, &flayer);

    if (pres != GST_VC1_PARSER_OK) {
      GST_ERROR_OBJECT (vc1parse, "Invalid VC1 frame layer");
      ret = GST_FLOW_ERROR;
      goto done;
    }

    GST_BUFFER_TIMESTAMP (buffer) =
        gst_util_uint64_scale (flayer.timestamp, GST_MSECOND, 1);
    if (!flayer.key)
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    else
      GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    /* For the simple/main profile this contains a single frame BDU without
     * startcodes and for the advanced profile this contains BDUs with
     * startcodes. In the case of the advanced profile parse them.
     *
     * Also for wrongly muxed simple/main profile streams with startcodes
     * we do the same.
     */
    if (startcodes) {
      /* skip frame layer header */
      if (!gst_vc1_parse_handle_bdus (vc1parse, buffer, 8, size - 8)) {
        ret = GST_FLOW_ERROR;
        goto done;
      }

      /* For the advanced profile we need a sequence header here */
      if (!vc1parse->seq_hdr_buffer) {
        GST_ERROR_OBJECT (vc1parse, "Need sequence header");
        ret = GST_FLOW_ERROR;
        goto done;
      }
    }
  }

  /* Need sequence header or sequence layer here, above code
   * checks this already */
  g_assert (vc1parse->seq_layer_buffer || vc1parse->seq_hdr_buffer);

  /* We need the entrypoint BDU for the advanced profile before we can set
   * the caps. For the ASF header format it will already be in the codec_data,
   * for the frame-layer stream format it will be in the first frame already.
   *
   * The only case where we wait another frame is the raw stream format, where
   * it will be the second BDU
   */
  if (vc1parse->profile == GST_VC1_PROFILE_ADVANCED
      && !vc1parse->entrypoint_buffer) {
    if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU
        || vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU_FRAME
        || vc1parse->input_stream_format == VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
        || vc1parse->input_stream_format ==
        VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME) {
      frame->flags |= GST_BASE_PARSE_FRAME_FLAG_QUEUE;
    } else {
      GST_ERROR_OBJECT (vc1parse, "Need entrypoint for the advanced profile");
      ret = GST_FLOW_ERROR;
      goto done;
    }
  }

  if (!gst_vc1_parse_update_caps (vc1parse)) {
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto done;
  }

  gst_buffer_unmap (buffer, &minfo);
  memset (&minfo, 0, sizeof (minfo));
  GST_DEBUG_OBJECT (vc1parse, "Finishing frame of size %" G_GSIZE_FORMAT,
      framesize);
  ret = gst_base_parse_finish_frame (parse, frame, framesize);

done:
  if (minfo.data)
    gst_buffer_unmap (buffer, &minfo);

  return ret;
}

static GstFlowReturn
gst_vc1_parse_push_sequence_and_entrypoint_headers (GstMfxVC1Parse * vc1parse)
{
  GstBuffer *codec_data;
  GstMapInfo minfo, sminfo, eminfo;

  /* Should have seqhdr and entrypoint for the advanced profile here */
  g_assert (vc1parse->seq_hdr_buffer && vc1parse->entrypoint_buffer);
  codec_data =
      gst_buffer_new_and_alloc (1 + 4 +
      gst_buffer_get_size (vc1parse->seq_hdr_buffer) + 4 +
      gst_buffer_get_size (vc1parse->entrypoint_buffer));

  gst_buffer_map (codec_data, &minfo, GST_MAP_WRITE);
  gst_buffer_map (vc1parse->seq_hdr_buffer, &sminfo, GST_MAP_READ);
  gst_buffer_map (vc1parse->entrypoint_buffer, &eminfo, GST_MAP_READ);

  if (vc1parse->profile == GST_VC1_PROFILE_SIMPLE)
    GST_WRITE_UINT8 (minfo.data, 0x29);
  else
    GST_WRITE_UINT8 (minfo.data, 0x2b);

  GST_WRITE_UINT32_BE (minfo.data + 1, 0x0000010f);
  memcpy (minfo.data + 1 + 4, sminfo.data, sminfo.size);
  GST_WRITE_UINT32_BE (minfo.data + 1 + 4 +
      gst_buffer_get_size (vc1parse->seq_hdr_buffer), 0x0000010e);
  memcpy (minfo.data + 1 + 4 + sminfo.size + 4, eminfo.data, eminfo.size);
  gst_buffer_unmap (codec_data, &minfo);
  gst_buffer_unmap (vc1parse->seq_hdr_buffer, &sminfo);
  gst_buffer_unmap (vc1parse->entrypoint_buffer, &eminfo);

  return gst_pad_push (GST_BASE_PARSE_SRC_PAD (vc1parse), codec_data);
}


static GstFlowReturn
gst_vc1_parse_push_sequence_layer (GstMfxVC1Parse * vc1parse)
{
  GstBuffer *seq_layer;

  if ((seq_layer = vc1parse->seq_layer_buffer))
    gst_buffer_ref (seq_layer);
  else
    seq_layer = gst_vc1_parse_make_sequence_layer (vc1parse);

  return gst_pad_push (GST_BASE_PARSE_SRC_PAD (vc1parse), seq_layer);
}

static GstFlowReturn
gst_vc1_parse_convert_asf_to_bdu (GstMfxVC1Parse * vc1parse,
    GstBaseParseFrame * frame)
{
  GstByteWriter bw;
  GstBuffer *buffer;
  GstBuffer *tmp;
  GstMemory *mem;
  guint8 sc_data[4];
  guint32 startcode;
  gboolean ok;
  GstFlowReturn ret = GST_FLOW_OK;

  buffer = frame->buffer;

  /* ASF frame could have a start code at the beginning or not. So we first
   * check for a start code if we have at least 4 bytes in the buffer. */
  if (gst_buffer_extract (buffer, 0, sc_data, 4) == 4) {
    startcode = GST_READ_UINT32_BE (sc_data);
    if (((startcode & 0xffffff00) == 0x00000100)) {
      /* Start code found */
      goto done;
    }
  }

  /* Yes, a frame could be smaller than 4 bytes and valid, for instance
   * black video. */

  /* We will prepend 4 bytes to buffer */
  gst_byte_writer_init_with_size (&bw, 4, TRUE);

  /* Set start code and suffixe, we assume raw asf data is a frame */
  ok = gst_byte_writer_put_uint24_be (&bw, 0x000001);
  ok &= gst_byte_writer_put_uint8 (&bw, 0x0D);
  tmp = gst_byte_writer_reset_and_get_buffer (&bw);

  /* Prepend startcode buffer to frame buffer */
  mem = gst_buffer_get_all_memory (tmp);
  gst_buffer_prepend_memory (buffer, mem);
  gst_buffer_unref (tmp);

  if (G_UNLIKELY (!ok)) {
    GST_ERROR_OBJECT (vc1parse, "convert asf to bdu failed");
    ret = GST_FLOW_ERROR;
  }

done:
  return ret;
}

static GstFlowReturn
gst_vc1_parse_convert_to_frame_layer (GstMfxVC1Parse * vc1parse,
    GstBaseParseFrame * frame)
{
  GstByteWriter bw;
  GstBuffer *buffer;
  GstBuffer *frame_layer;
  gsize frame_layer_size;
  GstMemory *mem;
  gboolean ok;
  gboolean keyframe;

  buffer = frame->buffer;
  keyframe = !(GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT));

  /* We need 8 bytes for frame-layer header */
  frame_layer_size = 8;

  gst_byte_writer_init_with_size (&bw, frame_layer_size, TRUE);

  /* frame-layer header shall be serialized in little-endian byte order */
  ok = gst_byte_writer_put_uint24_le (&bw, gst_buffer_get_size (buffer));

  if (keyframe)
    ok &= gst_byte_writer_put_uint8 (&bw, 0x80);        /* keyframe */
  else
    ok &= gst_byte_writer_put_uint8 (&bw, 0x00);

  ok &= gst_byte_writer_put_uint32_le (&bw, GST_BUFFER_PTS (buffer));

  frame_layer = gst_byte_writer_reset_and_get_buffer (&bw);
  mem = gst_buffer_get_all_memory (frame_layer);
  gst_buffer_prepend_memory (buffer, mem);
  gst_buffer_unref (frame_layer);

  if (G_UNLIKELY (!ok)) {
    GST_ERROR_OBJECT (vc1parse, "failed to convert to frame layer");
    return GST_FLOW_ERROR;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_vc1_parse_pre_push_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);
  GstFlowReturn ret = GST_FLOW_OK;

  if (!vc1parse->sent_codec_tag) {
    GstTagList *taglist;
    GstCaps *caps;

    caps = gst_pad_get_current_caps (GST_BASE_PARSE_SRC_PAD (parse));
    if (G_UNLIKELY (caps == NULL)) {
      if (GST_PAD_IS_FLUSHING (GST_BASE_PARSE_SRC_PAD (parse))) {
        GST_INFO_OBJECT (parse, "Src pad is flushing");
        return GST_FLOW_FLUSHING;
      } else {
        GST_INFO_OBJECT (parse, "Src pad is not negotiated!");
        return GST_FLOW_NOT_NEGOTIATED;
      }
    }

    taglist = gst_tag_list_new_empty ();

    /* codec tag */
    gst_pb_utils_add_codec_description_to_tag_list (taglist,
        GST_TAG_VIDEO_CODEC, caps);
    gst_caps_unref (caps);

    gst_base_parse_merge_tags (parse, taglist, GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (taglist);

    /* also signals the end of first-frame processing */
    vc1parse->sent_codec_tag = TRUE;
  }

  /* Nothing to do here */
  if (vc1parse->input_stream_format == vc1parse->output_stream_format)
    return GST_FLOW_OK;

  switch (vc1parse->output_stream_format) {
    case VC1_STREAM_FORMAT_BDU:
      switch (vc1parse->input_stream_format) {
        case VC1_STREAM_FORMAT_BDU:
          g_assert_not_reached ();
          break;
        case VC1_STREAM_FORMAT_BDU_FRAME:
          goto conversion_not_supported;
          break;
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU:
          /* We just need to drop sequence-layer buffer */
          if (frame->flags & GST_BASE_PARSE_FRAME_FLAG_NO_FRAME) {
            ret = GST_BASE_PARSE_FLOW_DROPPED;
          }
          break;
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME:
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME:
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER:
          goto conversion_not_supported;
          break;
        case VC1_STREAM_FORMAT_ASF:
          /* We need to send the sequence and entry headers first */
          if (!vc1parse->codec_data_sent) {
            ret = gst_vc1_parse_push_sequence_and_entrypoint_headers (vc1parse);
            if (ret != GST_FLOW_OK) {
              GST_ERROR_OBJECT (vc1parse, "push sequenceheaders failed");
              break;
            }
            vc1parse->codec_data_sent = TRUE;
          }
          ret = gst_vc1_parse_convert_asf_to_bdu (vc1parse, frame);
          break;
        case VC1_STREAM_FORMAT_FRAME_LAYER:
          goto conversion_not_supported;
          break;
        default:
          g_assert_not_reached ();
          break;
      }
      break;

    case VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER:
      switch (vc1parse->input_stream_format) {
        case VC1_STREAM_FORMAT_BDU:
        case VC1_STREAM_FORMAT_BDU_FRAME:
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU:
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME:
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME:
          goto conversion_not_supported;
          break;
        case VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER:
          g_assert_not_reached ();
          break;
        case VC1_STREAM_FORMAT_ASF:
          /* Make sure we push the sequence layer */
          if (!vc1parse->codec_data_sent) {
            ret = gst_vc1_parse_push_sequence_layer (vc1parse);
            if (ret != GST_FLOW_OK) {
              GST_ERROR_OBJECT (vc1parse, "push sequence layer failed");
              break;
            }
            vc1parse->codec_data_sent = TRUE;
          }
          ret = gst_vc1_parse_convert_to_frame_layer (vc1parse, frame);
          break;
        case VC1_STREAM_FORMAT_FRAME_LAYER:
          /* We just need to send the sequence-layer first */
          if (!vc1parse->codec_data_sent) {
            ret = gst_vc1_parse_push_sequence_layer (vc1parse);
            if (ret != GST_FLOW_OK) {
              GST_ERROR_OBJECT (vc1parse, "push sequence layer failed");
              break;
            }
            vc1parse->codec_data_sent = TRUE;
          }
          break;
        default:
          g_assert_not_reached ();
          break;
      }
      break;

    default:
      g_assert_not_reached ();
      break;
  }

  return ret;

conversion_not_supported:
  GST_WARNING_OBJECT (vc1parse, "stream conversion not implemented yet");
  return GST_FLOW_NOT_NEGOTIATED;
}

/* SMPTE 421M Table 7 */
static const struct
{
  gint par_n, par_d;
} aspect_ratios[] = {
  {
  0, 0}, {
  1, 1}, {
  12, 11}, {
  10, 11}, {
  16, 11}, {
  40, 33}, {
  24, 11}, {
  20, 11}, {
  32, 11}, {
  80, 33}, {
  18, 11}, {
  15, 11}, {
  64, 33}, {
  160, 99}, {
  0, 0}, {
  0, 0}
};

/* SMPTE 421M Table 8 */
static const guint framerates_n[] = {
  0,
  24 * 1000,
  25 * 1000, 30 * 1000, 50 * 1000, 60 * 1000, 48 * 1000, 72 * 1000
};

/* SMPTE 421M Table 9 */
static const guint framerates_d[] = {
  0,
  1000,
  1001
};

static gboolean
gst_vc1_parse_handle_seq_hdr (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size)
{
  GstVC1ParserResult pres;
  GstVC1Profile profile;
  GstMapInfo minfo;

  g_assert (gst_buffer_get_size (buf) >= offset + size);
  gst_buffer_replace (&vc1parse->seq_hdr_buffer, NULL);
  memset (&vc1parse->seq_hdr, 0, sizeof (vc1parse->seq_hdr));

  gst_buffer_map (buf, &minfo, GST_MAP_READ);
  pres =
      gst_vc1_parse_sequence_header (minfo.data + offset,
      size, &vc1parse->seq_hdr);
  gst_buffer_unmap (buf, &minfo);

  if (pres != GST_VC1_PARSER_OK) {
    GST_ERROR_OBJECT (vc1parse, "Invalid VC1 sequence header");
    return FALSE;
  }
  vc1parse->seq_hdr_buffer =
      gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL, offset, size);
  profile = vc1parse->seq_hdr.profile;
  if (vc1parse->profile != profile) {
    vc1parse->update_caps = TRUE;
    vc1parse->profile = vc1parse->seq_hdr.profile;
  }

  /* Only update fps if not from caps */
  if (!vc1parse->fps_from_caps && profile != GST_VC1_PROFILE_ADVANCED) {
    gint fps;
    /* This is only an estimate but better than nothing */
    fps = vc1parse->seq_hdr.struct_c.framerate;
    if (fps != 0 && (vc1parse->fps_d == 0 ||
            gst_util_fraction_compare (fps, 1, vc1parse->fps_n,
                vc1parse->fps_d) != 0)) {
      vc1parse->update_caps = TRUE;
      vc1parse->fps_n = fps;
      vc1parse->fps_d = 1;
    }
  }

  if (profile == GST_VC1_PROFILE_ADVANCED) {
    GstVC1Level level;
    gint width, height;
    level = vc1parse->seq_hdr.advanced.level;
    if (vc1parse->level != level) {
      vc1parse->update_caps = TRUE;
      vc1parse->level = level;
    }

    width = vc1parse->seq_hdr.advanced.max_coded_width;
    height = vc1parse->seq_hdr.advanced.max_coded_height;
    if (vc1parse->width != width || vc1parse->height != height) {
      vc1parse->update_caps = TRUE;
      vc1parse->width = width;
      vc1parse->height = height;
    }

    /* Only update fps if not from caps */
    if (!vc1parse->fps_from_caps) {
      gint fps;
      /* This is only an estimate but better than nothing */
      fps = vc1parse->seq_hdr.advanced.framerate;
      if (fps != 0 && (vc1parse->fps_d == 0 ||
              gst_util_fraction_compare (fps, 1, vc1parse->fps_n,
                  vc1parse->fps_d) != 0)) {
        vc1parse->update_caps = TRUE;
        vc1parse->fps_n = fps;
        vc1parse->fps_d = 1;
      }
    }

    if (vc1parse->seq_hdr.advanced.display_ext) {
      /* Only update PAR if not from input caps */
      if (!vc1parse->par_from_caps
          && vc1parse->seq_hdr.advanced.aspect_ratio_flag) {
        gint par_n, par_d;
        if (vc1parse->seq_hdr.advanced.aspect_ratio == 15) {
          par_n = vc1parse->seq_hdr.advanced.aspect_horiz_size;
          par_d = vc1parse->seq_hdr.advanced.aspect_vert_size;
        } else {
          par_n = aspect_ratios[vc1parse->seq_hdr.advanced.aspect_ratio].par_n;
          par_d = aspect_ratios[vc1parse->seq_hdr.advanced.aspect_ratio].par_d;
        }

        if (par_n != 0 && par_d != 0 &&
            (vc1parse->par_d == 0
                || gst_util_fraction_compare (par_n, par_d,
                    vc1parse->par_n, vc1parse->par_d) != 0)) {
          vc1parse->update_caps = TRUE;
          vc1parse->par_n = par_n;
          vc1parse->par_d = par_d;
        }
      }

      /* Only update fps if not from caps, better value than above */
      if (!vc1parse->fps_from_caps && vc1parse->seq_hdr.advanced.framerate_flag) {
        gint fps_n = 0, fps_d = 0;
        if (!vc1parse->seq_hdr.advanced.framerateind) {
          if (vc1parse->seq_hdr.advanced.frameratenr > 0
              && vc1parse->seq_hdr.advanced.frameratenr < 8
              && vc1parse->seq_hdr.advanced.frameratedr > 0
              && vc1parse->seq_hdr.advanced.frameratedr < 3) {
            fps_n = framerates_n[vc1parse->seq_hdr.advanced.frameratenr];
            fps_d = framerates_d[vc1parse->seq_hdr.advanced.frameratedr];
          }
        } else {
          fps_n = vc1parse->seq_hdr.advanced.framerateexp + 1;
          fps_d = 32;
        }

        if (fps_n != 0 && fps_d != 0 &&
            (vc1parse->fps_d == 0
                || gst_util_fraction_compare (fps_n, fps_d,
                    vc1parse->fps_n, vc1parse->fps_d) != 0)) {
          vc1parse->update_caps = TRUE;
          vc1parse->fps_n = fps_n;
          vc1parse->fps_d = fps_d;
        }
      }
    }
  }

  return TRUE;
}

static gboolean
gst_vc1_parse_handle_seq_layer (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size)
{
  GstVC1ParserResult pres;
  GstVC1Profile profile;
  GstVC1Level level;
  gint width, height;
  GstMapInfo minfo;

  g_assert (gst_buffer_get_size (buf) >= offset + size);

  gst_buffer_replace (&vc1parse->seq_layer_buffer, NULL);
  memset (&vc1parse->seq_layer, 0, sizeof (vc1parse->seq_layer));

  gst_buffer_map (buf, &minfo, GST_MAP_READ);
  pres =
      gst_vc1_parse_sequence_layer (minfo.data + offset,
      size, &vc1parse->seq_layer);
  gst_buffer_unmap (buf, &minfo);

  if (pres != GST_VC1_PARSER_OK) {
    GST_ERROR_OBJECT (vc1parse, "Invalid VC1 sequence layer");
    return FALSE;
  }
  vc1parse->seq_layer_buffer =
      gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL, offset, size);
  profile = vc1parse->seq_layer.struct_c.profile;
  if (vc1parse->profile != profile) {
    vc1parse->update_caps = TRUE;
    vc1parse->profile = vc1parse->seq_layer.struct_c.profile;
  }

  width = vc1parse->seq_layer.struct_a.vert_size;
  height = vc1parse->seq_layer.struct_a.horiz_size;
  if (width > 0 && height > 0
      && (vc1parse->width != width || vc1parse->height != height)) {
    vc1parse->update_caps = TRUE;
    vc1parse->width = width;
    vc1parse->height = height;
  }

  level = vc1parse->seq_layer.struct_b.level;
  if (vc1parse->level != level) {
    vc1parse->update_caps = TRUE;
    vc1parse->level = level;
  }

  if (!vc1parse->fps_from_caps && profile != GST_VC1_PROFILE_ADVANCED) {
    gint fps;
    fps = vc1parse->seq_layer.struct_c.framerate;
    if (fps == 0 || fps == -1)
      fps = vc1parse->seq_layer.struct_b.framerate;
    if (fps != 0 && fps != -1 && (vc1parse->fps_d == 0 ||
            gst_util_fraction_compare (fps, 1, vc1parse->fps_n,
                vc1parse->fps_d) != 0)) {
      vc1parse->update_caps = TRUE;
      vc1parse->fps_n = fps;
      vc1parse->fps_d = 1;
    }
  }

  /* And now update the duration */
  if (vc1parse->seq_layer.numframes != 0 && vc1parse->seq_layer.numframes != -1)
    gst_base_parse_set_duration (GST_BASE_PARSE (vc1parse),
        GST_FORMAT_DEFAULT, vc1parse->seq_layer.numframes, 50);
  return TRUE;
}

static gboolean
gst_vc1_parse_handle_entrypoint (GstMfxVC1Parse * vc1parse,
    GstBuffer * buf, guint offset, guint size)
{
  g_assert (gst_buffer_get_size (buf) >= offset + size);

  gst_buffer_replace (&vc1parse->entrypoint_buffer, NULL);
  vc1parse->entrypoint_buffer =
      gst_buffer_copy_region (buf, GST_BUFFER_COPY_ALL, offset, size);

  return TRUE;
}

static void
gst_vc1_parse_update_stream_format_properties (GstMfxVC1Parse * vc1parse)
{
  if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU
      || vc1parse->input_stream_format == VC1_STREAM_FORMAT_BDU_FRAME) {
    /* Need at least the 4 bytes start code */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 4);
    gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), TRUE);
  } else
      if (vc1parse->input_stream_format ==
      VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU
      || vc1parse->input_stream_format ==
      VC1_STREAM_FORMAT_SEQUENCE_LAYER_BDU_FRAME) {
    /* Need at least the 36 bytes sequence layer */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 36);
    gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), TRUE);
  } else
      if (vc1parse->input_stream_format ==
      VC1_STREAM_FORMAT_SEQUENCE_LAYER_RAW_FRAME
      || vc1parse->input_stream_format ==
      VC1_STREAM_FORMAT_SEQUENCE_LAYER_FRAME_LAYER) {
    /* Need at least the 36 bytes sequence layer */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 36);
    gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), FALSE);
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_ASF) {
    vc1parse->input_stream_format = VC1_STREAM_FORMAT_ASF;
    /* Need something, assume this is already packetized */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 1);
    gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), FALSE);
  } else if (vc1parse->input_stream_format == VC1_STREAM_FORMAT_FRAME_LAYER) {
    /* Need at least the frame layer header */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (vc1parse), 8);
    gst_base_parse_set_syncable (GST_BASE_PARSE (vc1parse), FALSE);
  } else {
    g_assert_not_reached ();
  }
}

static gboolean
gst_vc1_parse_set_caps (GstBaseParse * parse, GstCaps * caps)
{
  GstMfxVC1Parse *vc1parse = GST_VC1_PARSE (parse);
  GstStructure *s;
  const GValue *value;
  GstBuffer *codec_data = NULL;
  const gchar *stream_format = NULL;
  const gchar *header_format = NULL;
  const gchar *profile = NULL;
  const gchar *format;

  GST_DEBUG_OBJECT (parse, "caps %" GST_PTR_FORMAT, caps);
  /* Parse the caps to get as much information as possible */
  s = gst_caps_get_structure (caps, 0);

  vc1parse->width = 0;
  gst_structure_get_int (s, "width", &vc1parse->width);
  vc1parse->height = 0;
  gst_structure_get_int (s, "height", &vc1parse->height);

  vc1parse->fps_n = vc1parse->fps_d = 0;
  vc1parse->fps_from_caps = FALSE;
  gst_structure_get_fraction (s, "framerate", &vc1parse->fps_n,
      &vc1parse->fps_d);
  if (vc1parse->fps_d != 0)
    vc1parse->fps_from_caps = TRUE;

  gst_structure_get_fraction (s, "pixel-aspect-ratio",
      &vc1parse->par_n, &vc1parse->par_d);
  if (vc1parse->par_n != 0 && vc1parse->par_d != 0)
    vc1parse->par_from_caps = TRUE;

  vc1parse->format = 0;
  format = gst_structure_get_string (s, "format");
  if (format && strcmp (format, "WVC1") == 0)
    vc1parse->format = GST_VC1_PARSE_FORMAT_WVC1;
  else
    vc1parse->format = GST_VC1_PARSE_FORMAT_WMV3;

  vc1parse->profile = -1;
  profile = gst_structure_get_string (s, "profile");
  if (profile && strcmp (profile, "simple"))
    vc1parse->profile = GST_VC1_PROFILE_SIMPLE;
  else if (profile && strcmp (profile, "main"))
    vc1parse->profile = GST_VC1_PROFILE_MAIN;
  else if (profile && strcmp (profile, "advanced"))
    vc1parse->profile = GST_VC1_PROFILE_ADVANCED;
  else if (vc1parse->format == GST_VC1_PARSE_FORMAT_WVC1)
    vc1parse->profile = GST_VC1_PROFILE_ADVANCED;
  else if (vc1parse->format == GST_VC1_PARSE_FORMAT_WMV3)
    vc1parse->profile = GST_VC1_PROFILE_MAIN;   /* or SIMPLE */

  vc1parse->level = -1;
  vc1parse->detecting_stream_format = FALSE;
  header_format = gst_structure_get_string (s, "header-format");
  stream_format = gst_structure_get_string (s, "stream-format");

  /* Now parse the codec_data */
  gst_buffer_replace (&vc1parse->seq_layer_buffer, NULL);
  gst_buffer_replace (&vc1parse->seq_hdr_buffer, NULL);
  gst_buffer_replace (&vc1parse->entrypoint_buffer, NULL);
  memset (&vc1parse->seq_layer, 0, sizeof (vc1parse->seq_layer));
  memset (&vc1parse->seq_hdr, 0, sizeof (vc1parse->seq_hdr));
  value = gst_structure_get_value (s, "codec_data");

  if (value != NULL) {
    gsize codec_data_size;
    GstMapInfo minfo;

    codec_data = gst_value_get_buffer (value);
    gst_buffer_map (codec_data, &minfo, GST_MAP_READ);
    codec_data_size = gst_buffer_get_size (codec_data);
    if ((codec_data_size == 4 || codec_data_size == 5)) {
      /* ASF, VC1/WMV3 simple/main profile
       * This is the sequence header without start codes
       */
      if (!gst_vc1_parse_handle_seq_hdr (vc1parse, codec_data,
              0, codec_data_size)) {
        gst_buffer_unmap (codec_data, &minfo);
        return FALSE;
      }
      if (header_format && strcmp (header_format, "asf") != 0)
        GST_WARNING_OBJECT (vc1parse,
            "Upstream claimed '%s' header format but 'asf' detected",
            header_format);
      vc1parse->input_header_format = VC1_HEADER_FORMAT_ASF;
    } else if (codec_data_size == 36 && minfo.data[3] == 0xc5) {
      /* Sequence Layer, SMPTE S421M-2006 Annex L.3 */
      if (!gst_vc1_parse_handle_seq_layer (vc1parse, codec_data, 0,
              codec_data_size)) {
        GST_ERROR_OBJECT (vc1parse, "Invalid VC1 sequence layer");
        gst_buffer_unmap (codec_data, &minfo);
        return FALSE;
      }

      if (header_format && strcmp (header_format, "sequence-layer") != 0)
        GST_WARNING_OBJECT (vc1parse,
            "Upstream claimed '%s' header format but 'sequence-layer' detected",
            header_format);
      vc1parse->input_header_format = VC1_HEADER_FORMAT_SEQUENCE_LAYER;
    } else {
      gint offset;
      guint32 start_code;
      /* ASF, VC1 advanced profile
       * This should be the
       * 1) ASF binding byte
       * 2) Sequence Header with startcode
       * 3) EntryPoint Header with startcode
       */
      if (codec_data_size < 4 + 2) {
        GST_ERROR_OBJECT (vc1parse,
            "Too small for VC1 advanced profile ASF header");
        gst_buffer_unmap (codec_data, &minfo);
        return FALSE;
      }

      /* Some sanity checking */
      if ((minfo.data[0] & 0x01) != 0x01) {
        GST_WARNING_OBJECT (vc1parse,
            "Invalid binding byte for VC1 advanced profile ASF header");
        offset = 0;
      } else {
        offset = 1;
      }

      start_code = GST_READ_UINT32_BE (minfo.data + offset);
      if (start_code != 0x000010f) {
        GST_WARNING_OBJECT (vc1parse,
            "VC1 advanced profile ASF header does not start with SequenceHeader startcode");
      }

      if (!gst_vc1_parse_handle_bdus (vc1parse, codec_data, offset,
              codec_data_size - offset)) {
        gst_buffer_unmap (codec_data, &minfo);
        if (!gst_vc1_parse_handle_bdu (vc1parse, GST_VC1_SEQUENCE, codec_data,
            0, codec_data_size))
          return FALSE;
        gst_buffer_map (codec_data, &minfo, GST_MAP_READ);
      }

      if (!vc1parse->seq_hdr_buffer) {
        GST_ERROR_OBJECT (vc1parse,
            "Need sequence header in the codec_data");
        gst_buffer_unmap (codec_data, &minfo);
        return FALSE;
      }

      if (vc1parse->profile == GST_VC1_PROFILE_ADVANCED
          && !vc1parse->entrypoint_buffer) {
         GST_ERROR_OBJECT (vc1parse,
            "Need entrypoint header in the codec_data for advanced profile");
        gst_buffer_unmap (codec_data, &minfo);
        return FALSE;
      }

      if (header_format && strcmp (header_format, "asf") != 0)
        GST_WARNING_OBJECT (vc1parse,
            "Upstream claimed '%s' header format but 'asf' detected",
            header_format);

      vc1parse->input_header_format = VC1_HEADER_FORMAT_ASF;
    }
    gst_buffer_unmap (codec_data, &minfo);
  } else {
    vc1parse->input_header_format = VC1_HEADER_FORMAT_NONE;
    if (header_format && strcmp (header_format, "none") != 0)
      GST_WARNING_OBJECT (vc1parse,
          "Upstream claimed '%s' header format but 'none' detected",
          header_format);
  }

  /* If no stream-format was set we try to detect it */
  if (!stream_format) {
    vc1parse->detecting_stream_format = TRUE;
  } else {
    vc1parse->input_stream_format = stream_format_from_string (stream_format);
    gst_vc1_parse_update_stream_format_properties (vc1parse);
  }

  vc1parse->renegotiate = TRUE;
  vc1parse->update_caps = TRUE;
  return TRUE;
}
