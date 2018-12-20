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


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <interface/videodefs.h>
#include "resmemory.h"
#include "wssink.h"


GST_DEBUG_CATEGORY_STATIC (gst_ws_sink_debug);
#define GST_CAT_DEFAULT gst_ws_sink_debug

#define GST_WS_SINK_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_WS_SINK, GstWsSinkPrivate))

struct _GstWsSinkPrivate
{
    /* number of received and processed buffers */
    guint64 bit_received_num;
    guint64 bit_processed_num;
    guint64 txt_received_num;
    guint64 txt_processed_num;
};

/* WsSink properties */
enum
{
    PROP_0,
    PROP_WS_SERVER_URI,
    PROP_WS_CLIENT_ID,
    PROP_WS_CLIENT_PROXY,
    PROP_LAST
};

#define INVALID_WSC_PROXY  ((void *)(-1))
#define TXT_BUFFER_SIZE_MAX 1024

static const char gst_ws_bit_src_caps_str[] = "image/jpeg";
static GstStaticPadTemplate gst_ws_bit_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_bit",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (gst_ws_bit_src_caps_str));

static const char gst_ws_txt_src_caps_str[] = "data/res_txt";
static GstStaticPadTemplate gst_ws_txt_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_txt",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (gst_ws_txt_src_caps_str));
    /*GST_STATIC_CAPS_ANY);*/

static GstElementClass *parent_class = NULL;

static void gst_ws_sink_get_times (GstWsSink * basesink, GstBuffer * buffer, GstClockTime * start);

#define INVALID_BUFFER_TS (-1)
// get the bit buffer and txt buffer with the same ts
// if not, find the earliest buffer as the output
static gboolean get_buffers_with_same_ts(GstWsSink * basesink, GstBuffer **bit_buf, GstBuffer **txt_buf)
{
    GstBuffer *bit_buf_temp, *txt_buf_temp;
    GstClockTime bit_ts=INVALID_BUFFER_TS;
    GstClockTime txt_ts=INVALID_BUFFER_TS;
    GstWsSinkPrivate *priv = basesink->priv;

    txt_buf_temp = NULL;
    bit_buf_temp = NULL;

    // try to get a buffer of bitstream
    GST_WS_SINK_BIT_LOCK(basesink);
    if(gst_buffer_list_length(basesink->bit_list)>0) {
        bit_buf_temp = gst_buffer_list_get(basesink->bit_list,0);
        gst_ws_sink_get_times(basesink, bit_buf_temp, &bit_ts);
    }
    GST_WS_SINK_BIT_UNLOCK(basesink);

    // try to get a buffer of txt data
    GST_WS_SINK_TXT_LOCK(basesink);
    if(gst_buffer_list_length(basesink->txt_list)>0) {
        txt_buf_temp = gst_buffer_list_get(basesink->txt_list,0);
        gst_ws_sink_get_times(basesink, txt_buf_temp, &txt_ts);
    }
    GST_WS_SINK_TXT_UNLOCK(basesink);

    // TODO: align bit buffer and txt buffer

    // can not get any, return NULL
    if(G_UNLIKELY((!bit_buf_temp && !txt_buf_temp))) {
        *txt_buf = NULL;
        *bit_buf = NULL;
        return FALSE;
    }

    // bit buffer is early, only return it
    if(G_UNLIKELY(bit_ts < txt_ts)){
        GST_WS_SINK_BIT_LOCK(basesink);
        gst_buffer_list_remove(basesink->bit_list, 0, 1);
        GST_WS_SINK_BIT_UNLOCK(basesink);
        priv->bit_processed_num++;
        *txt_buf = NULL;
        *bit_buf = bit_buf_temp;
        return TRUE;
    }

    // txt buffer is early, only return it
    if(G_UNLIKELY(txt_ts < bit_ts)){
        GST_WS_SINK_TXT_LOCK(basesink);
        gst_buffer_list_remove(basesink->txt_list, 0, 1);
        GST_WS_SINK_TXT_UNLOCK(basesink);
        priv->txt_processed_num++;
        *txt_buf = txt_buf_temp;
        *bit_buf = NULL;
        return TRUE;
    }

    GST_WS_SINK_BIT_LOCK(basesink);
    gst_buffer_list_remove(basesink->bit_list, 0, 1);
    GST_WS_SINK_BIT_UNLOCK(basesink);
    priv->bit_processed_num++;

    GST_WS_SINK_TXT_LOCK(basesink);
    gst_buffer_list_remove(basesink->txt_list, 0, 1);
    GST_WS_SINK_TXT_UNLOCK(basesink);
    priv->txt_processed_num++;

    *txt_buf = txt_buf_temp;
    *bit_buf = bit_buf_temp;

    return TRUE;
}
// Main function to processed buffer into next filter
//      1. package jpeg bitstream and inference result data
//      2. send out by WebSocket
static void process_sink_buffers(gpointer userData)
{
    GstWsSink *basesink;
    basesink = GST_WS_SINK (userData);
    GstBuffer *bit_buf = NULL, *txt_buf = NULL;
    gboolean ret = FALSE;
    int i = 0;
    gchar txt_cache[TXT_BUFFER_SIZE_MAX]={0};
    gsize data_len = 0, size = 0;
    //GstWsSinkPrivate *priv = basesink->priv;

    ret = get_buffers_with_same_ts(basesink, &bit_buf, &txt_buf);
    if(ret==FALSE) {
        g_usleep(10000);
        return;
    }

    //setup wsclient
    if(!basesink->wsclient_handle) {
        if(basesink->wsclient_handle_proxy != INVALID_WSC_PROXY)
               basesink->wsclient_handle = basesink->wsclient_handle_proxy;
        else
                basesink->wsclient_handle = wsclient_setup(basesink->wss_uri, basesink->wsc_id);
        wsclient_set_id(basesink->wsclient_handle,  basesink->wsc_id);
    }

    // txt data
    if(txt_buf) {
        ResMemory *txt_mem;
        txt_mem = RES_MEMORY_CAST(res_memory_acquire(txt_buf));
        if(!txt_mem || !txt_mem->data || !txt_mem->data_count) {
            g_print("Failed to get data from txt buffer!!!\n");
        }else{
            // packaged txt data into websocket
            InferenceData *infer_data = txt_mem->data;
            int count = txt_mem->data_count;
            GST_LOG("object num = %d\n",count);
            for(i=0; i<count; i++) {
                data_len = g_snprintf(txt_cache, TXT_BUFFER_SIZE_MAX,
                    "ts=%.3fs, prob=%f,name=%s, rect=(%d,%d)@%dx%d\n",
                    txt_mem->pts/1000000000.0, infer_data->probility, infer_data->label,
                    infer_data->rect.x, infer_data->rect.y,
                    infer_data->rect.width, infer_data->rect.height);
                basesink->data_index++;
                g_print("pipe %d:  send %2d xml_data: size=%ld, %s",basesink->wsc_id, basesink->data_index, data_len, txt_cache);
                wsclient_send_data(basesink->wsclient_handle, (char *)txt_cache, data_len);
                size += data_len;
                infer_data++;
             }
        }
    }

    // jpeg bitstream data
    if(bit_buf) {
        int n = gst_buffer_n_memory (bit_buf);
        GstMemory *mem = NULL;
        gpointer data_base = 0;
        size = 0;
        // It will be released automatically when the current stack frame is cleaned up
        GstMapInfo *mapInfo = g_newa (GstMapInfo, n);
        GST_LOG("bitstream block num = %d\n",n);
        for (i = 0; i < n; ++i) {
            mem = gst_buffer_peek_memory (bit_buf, i);
            if (gst_memory_map (mem, &mapInfo[i], GST_MAP_READ)) {
                data_base = mapInfo[i].data;
                data_len = mapInfo[i].size;
                wsclient_send_data(basesink->wsclient_handle, (char *)data_base, data_len);
            }
            size += data_len;
            //g_print("ws send bit_data: size=%ld, ts=%.3fs, data=%p\n\n", data_len,
            //    GST_BUFFER_PTS(bit_buf)/1000000000.0, data_base);
        }

        for (i = 0; i < n; ++i)
            gst_memory_unmap (mapInfo[i].memory, &mapInfo[i]);
   }

    // release buffer
    if(bit_buf)
        gst_buffer_unref(bit_buf);
    if(txt_buf)
        gst_buffer_unref(txt_buf);
}

static GstCaps *
gst_ws_sink_query_caps (GstWsSink * bsink, GstPad * pad, GstCaps * filter)
{
    GstCaps *caps = NULL, *temp=NULL;
    GstPadDirection direction = GST_PAD_DIRECTION (pad);

    if(direction==GST_PAD_SRC) {
        g_print("It has no src pad!!!\n");
        return NULL;
    }

    if(filter) {
        temp = gst_pad_get_pad_template_caps (pad);
        caps = gst_caps_intersect_full (filter, temp, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(temp);
    } else {
        caps = gst_pad_get_pad_template_caps (pad);
    }

    GST_LOG("filter_caps = %s\n", gst_caps_to_string(filter));
    GST_LOG("pad_caps = %s\n", gst_caps_to_string(temp));
    GST_LOG("caps = %s\n", gst_caps_to_string(caps));

    return caps;
}

static GstCaps *
gst_ws_sink_fixate (GstWsSink * bsink, GstCaps * caps)
{
    GST_DEBUG_OBJECT (bsink, "using default caps fixate function");

    caps = gst_caps_fixate (caps);
    return caps;
}

static gboolean
gst_ws_sink_propose_allocation (GstWsSink *sink, GstQuery *query)
{
    /* propose allocation parameters for upstream */
    return FALSE;
}

static void
gst_ws_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstWsSink *sink = GST_WS_SINK (object);

  switch (prop_id) {
    case PROP_WS_SERVER_URI:
        sink->wss_uri = g_value_dup_string (value);
        break;
    case PROP_WS_CLIENT_ID:
        sink->wsc_id = g_value_get_int(value);
        break;
     case PROP_WS_CLIENT_PROXY:
        sink->wsclient_handle_proxy=g_value_get_pointer(value);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ws_sink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstWsSink *sink = GST_WS_SINK (object);

  switch (prop_id) {
    case PROP_WS_SERVER_URI:
        g_value_set_string (value, sink->wss_uri);
        break;
    case PROP_WS_CLIENT_ID:
        g_value_set_int (value, sink->wsc_id);
        break;
    case PROP_WS_CLIENT_PROXY:
        g_value_set_pointer(value, sink->wsclient_handle_proxy);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_ws_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstWsSink *basesink;
  gboolean result = TRUE;

  basesink = GST_WS_SINK_CAST (parent);
  //bclass = GST_WS_SINK_GET_CLASS (basesink);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      //GstCaps *caps;
      GST_DEBUG_OBJECT (basesink, "caps %p", event);

      // we will not replace the caps
      //gst_event_parse_caps (event, &caps);
      break;
    }
    case GST_EVENT_TAG:
    {
      GstTagList *taglist;

      gst_event_parse_tag (event, &taglist);

      gst_element_post_message (GST_ELEMENT_CAST (basesink),
          gst_message_new_tag (GST_OBJECT_CAST (basesink),
              gst_tag_list_copy (taglist)));
      break;
    }
    case GST_EVENT_SINK_MESSAGE:
    {
      GstMessage *msg = NULL;

      gst_event_parse_sink_message (event, &msg);
      if (msg)
        gst_element_post_message (GST_ELEMENT_CAST (basesink), msg);
      break;
    }
    case GST_EVENT_EOS:
    {
      // EOS message is used to trigger the state change
      GstMessage *message = gst_message_new_eos (GST_OBJECT_CAST (basesink));
      //gst_message_set_seqnum (message, seqnum);
      gst_element_post_message (GST_ELEMENT_CAST (basesink), message);
      break;
    }
    default:
      //GST_ELEMENT_CAST (basesink)->event (GST_ELEMENT_CAST (basesink), event);
      break;
  }

  gst_event_unref (event);

  return result;
}

/* get the timestamps on this buffer */
static void gst_ws_sink_get_times (GstWsSink * basesink, GstBuffer * buffer, GstClockTime * start)
{
  GstClockTime timestamp;

  /* first sync on DTS, else use PTS */
  timestamp = GST_BUFFER_DTS (buffer);
  if (!GST_CLOCK_TIME_IS_VALID (timestamp))
    timestamp = GST_BUFFER_PTS (buffer);

  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    *start = timestamp;
  } else {
    *start = 0;
  }
}


static GstFlowReturn
gst_ws_sink_bit_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstWsSink *basesink;
    basesink = GST_WS_SINK (parent);
    GstWsSinkPrivate *priv = basesink->priv;

    // put buffer into bufferlist
    GST_WS_SINK_BIT_LOCK(parent);
    buf = gst_buffer_ref(buf);
    gst_buffer_list_add(basesink->bit_list, buf);
    priv->bit_received_num++;
    GST_WS_SINK_BIT_UNLOCK(parent);

    return GST_FLOW_OK;
}

static GstFlowReturn
gst_ws_sink_txt_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstWsSink *basesink;
    basesink = GST_WS_SINK (parent);
    GstWsSinkPrivate *priv = basesink->priv;

    // put buffer into bufferlist
    GST_WS_SINK_TXT_LOCK(parent);
    buf = gst_buffer_ref(buf);
    gst_buffer_list_add(basesink->txt_list, buf);
    priv->txt_received_num++;
    GST_WS_SINK_TXT_UNLOCK(parent);

    return GST_FLOW_OK;
}


static gboolean
gst_ws_sink_pad_activate (GstPad * pad, GstObject * parent)
{
  gboolean result = FALSE;
  GstWsSink *basesink;

  basesink = GST_WS_SINK (parent);

  /* push mode */
  GST_DEBUG_OBJECT (basesink, "Set push mode");
  if ((result = gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE))) {
    GST_DEBUG_OBJECT (basesink, "Success activating push mode");
  }

  if (!result) {
    GST_WARNING_OBJECT (basesink, "Could not activate pad in either mode");
  }

  return result;
}


static gboolean
gst_ws_sink_pad_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;

  switch (mode) {
    case GST_PAD_MODE_PUSH:
      res = TRUE;
      break;
    default:
      GST_LOG_OBJECT (pad, "unknown activation mode %d", mode);
      res = FALSE;
      break;
  }
  return res;
}

/* send an event to our sinkpad peer. */
static gboolean
gst_ws_sink_send_event (GstElement * element, GstEvent * event)
{
  GstWsSink *basesink = GST_WS_SINK (element);
  gboolean result = TRUE;

  GST_DEBUG_OBJECT (basesink, "handling event %p %" GST_PTR_FORMAT, event,
      event);
  // The 2 sink pad will not send any event to upstream pad.
  return result;
}

static gboolean
default_element_query (GstElement * element, GstQuery * query)
{
  gboolean res = FALSE;
  GstWsSink *basesink = GST_WS_SINK (element);

  // TODO: how to query the 2 sink pad?
  res = gst_pad_peer_query (basesink->sinkpad_bit, query);
  GST_DEBUG_OBJECT (basesink, "query %s returns %d",
      GST_QUERY_TYPE_NAME (query), res);
  return res;
}

static gboolean
//gst_ws_sink_default_query (GstWsSink * basesink, GstPad *pad, GstQuery * query)
gst_ws_sink_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstWsSink *basesink;
  //GstWsSinkClass *bclass;

  basesink = GST_WS_SINK_CAST (parent);
  //bclass = GST_WS_SINK_GET_CLASS (basesink);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:
    {
      res = gst_ws_sink_propose_allocation (basesink, query);
      break;
    }
    case GST_QUERY_CAPS:
    {
      GstCaps *caps, *filter;

      gst_query_parse_caps (query, &filter);
      caps = gst_ws_sink_query_caps (basesink, pad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    case GST_QUERY_ACCEPT_CAPS:
    {
      GstCaps *caps, *allowed;
      gboolean subset;

      /* slightly faster than the default implementation */
      gst_query_parse_accept_caps (query, &caps);
      allowed = gst_ws_sink_query_caps (basesink, pad, NULL);
      subset = gst_caps_is_subset (caps, allowed);
      GST_DEBUG_OBJECT (basesink, "Checking if requested caps %" GST_PTR_FORMAT
          " are a subset of pad caps %" GST_PTR_FORMAT " result %d", caps,
          allowed, subset);
      gst_caps_unref (allowed);
      gst_query_set_accept_caps_result (query, subset);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, GST_OBJECT_CAST (basesink), query);
      break;
  }
  return res;
}

static GstStateChangeReturn
gst_ws_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstWsSink *basesink = GST_WS_SINK (element);
  //GstWsSinkClass *bclass;
  //bclass = GST_WS_SINK_GET_CLASS (basesink);

  switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            GST_DEBUG_OBJECT (basesink, "NULL to READY");
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            GST_DEBUG_OBJECT (basesink, "READY to PAUSED");
            break;
         case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            GST_DEBUG_OBJECT (basesink, "PAUSED to PLAYING, don't need preroll");
            break;
        default:
            break;
  }

  GstStateChangeReturn bret;
  bret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (G_UNLIKELY (bret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT (basesink,
                "element failed to change states -- activation problem?");
        return GST_STATE_CHANGE_FAILURE;
  }

  switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            GST_DEBUG_OBJECT (basesink, "PLAYING to PAUSED");
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_DEBUG_OBJECT (basesink, "PAUSED to READY");
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_DEBUG_OBJECT (basesink, "READY to NULL");
            break;
        default:
            break;
  }

  return ret;
}

static void gst_ws_sink_finalize (GObject * object);
static void
gst_ws_sink_class_init (GstWsSinkClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);

    GST_DEBUG_CATEGORY_INIT (gst_ws_sink_debug, "basesink", 0,
        "basesink element");

    g_type_class_add_private (klass, sizeof (GstWsSinkPrivate));
    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gst_ws_sink_finalize;
    gobject_class->set_property = gst_ws_sink_set_property;
    gobject_class->get_property = gst_ws_sink_get_property;

    gst_element_class_set_details_simple (gstelement_class,
      "WsSink", "Sink",
      "Send inference data(jpeg and parameters) based on WebSocket",
      "River,Li <river.li@intel.com>");

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WS_SERVER_URI,
      g_param_spec_string ("wssuri", "WebSocketUri",
          "The URI of WebSocket Server", "wss://localhost:8123/binaryEchoWithSize",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WS_CLIENT_ID,
      g_param_spec_int ("wsclientid", "WS client Index",
          "WebSocket client index to connected to WebSocket server(default: 0)",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WS_CLIENT_PROXY,
      g_param_spec_pointer ("wsclientproxy", "WS client proxy",
          "WebSocket client proxy to connected to WebSocket server.",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /* src pad */
    gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ws_bit_src_factory));
    gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ws_txt_src_factory));

    gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_ws_sink_change_state);
    gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_ws_sink_send_event);
    gstelement_class->query = GST_DEBUG_FUNCPTR (default_element_query);

    /* Registering debug symbols for function pointers */
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_fixate);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_pad_activate);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_pad_activate_mode);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_event);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_bit_chain);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_txt_chain);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ws_sink_sink_query);
}


static void
gst_ws_sink_init (GstWsSink * basesink, gpointer g_class)
{
    GstPadTemplate *pad_template;
    GstWsSinkPrivate *priv;

    GST_DEBUG_CATEGORY_INIT (gst_ws_sink_debug, "wssink", 0,
            "Send data out by WebSocket");

    basesink->wsclient_handle = NULL;
    basesink->wsclient_handle_proxy = INVALID_WSC_PROXY;
    basesink->wss_uri = NULL;
    basesink->wsc_id = 0;
    basesink->data_index = 0;

    basesink->priv = priv = GST_WS_SINK_GET_PRIVATE (basesink);

    priv->bit_received_num = 0;
    priv->bit_processed_num = 0;
    priv->txt_received_num = 0;
    priv->txt_processed_num = 0;

    g_mutex_init (&basesink->lock);
    g_cond_init (&basesink->cond);
    g_mutex_init (&basesink->bit_lock);
    g_mutex_init (&basesink->txt_lock);

    pad_template =
        gst_element_class_get_pad_template (GST_ELEMENT_CLASS (g_class), "sink_bit");
    g_return_if_fail (pad_template != NULL);
    basesink->sinkpad_bit = gst_pad_new_from_template (pad_template, "sink_bit");
    gst_pad_use_fixed_caps(basesink->sinkpad_bit);

    pad_template =
        gst_element_class_get_pad_template (GST_ELEMENT_CLASS (g_class), "sink_txt");
    g_return_if_fail (pad_template != NULL);
    basesink->sinkpad_txt = gst_pad_new_from_template (pad_template, "sink_txt");
    gst_pad_use_fixed_caps(basesink->sinkpad_txt);

    gst_pad_set_activate_function (basesink->sinkpad_bit, gst_ws_sink_pad_activate);
    gst_pad_set_activatemode_function (basesink->sinkpad_bit, gst_ws_sink_pad_activate_mode);
    gst_pad_set_query_function (basesink->sinkpad_bit, gst_ws_sink_sink_query);
    gst_pad_set_event_function (basesink->sinkpad_bit, gst_ws_sink_event);
    gst_pad_set_chain_function (basesink->sinkpad_bit, gst_ws_sink_bit_chain);
    gst_element_add_pad (GST_ELEMENT_CAST (basesink), basesink->sinkpad_bit);

    gst_pad_set_activate_function (basesink->sinkpad_txt, gst_ws_sink_pad_activate);
    gst_pad_set_activatemode_function (basesink->sinkpad_txt, gst_ws_sink_pad_activate_mode);
    gst_pad_set_query_function (basesink->sinkpad_txt, gst_ws_sink_sink_query);
    gst_pad_set_event_function (basesink->sinkpad_txt, gst_ws_sink_event);
    gst_pad_set_chain_function (basesink->sinkpad_txt, gst_ws_sink_txt_chain);
    gst_element_add_pad (GST_ELEMENT_CAST (basesink), basesink->sinkpad_txt);

    GST_OBJECT_FLAG_SET (basesink, GST_ELEMENT_FLAG_SINK);

    // create buffer list
    basesink->bit_list = gst_buffer_list_new ();
    basesink->txt_list = gst_buffer_list_new ();

    // create task for push data to next filter/element
    g_rec_mutex_init (&basesink->task_lock);
    basesink->task = gst_task_new (process_sink_buffers, (gpointer)basesink, NULL);
    gst_task_set_lock (basesink->task, &basesink->task_lock);
    gst_task_set_enter_callback (basesink->task, NULL, NULL, NULL);
    gst_task_set_leave_callback (basesink->task, NULL, NULL, NULL);

    // start task
    gst_task_start(basesink->task);
    
}

static void
gst_ws_sink_finalize (GObject * object)
{
    GstWsSink *basesink;
    basesink = GST_WS_SINK (object);

   if((basesink->wsclient_handle_proxy==INVALID_WSC_PROXY)
    &&( basesink->wsclient_handle) ) {
        wsclient_destroy(basesink->wsclient_handle);
        basesink->wsclient_handle = NULL;
    }

    gst_task_stop(basesink->task);
    gst_task_join(basesink->task);
    gst_object_unref(basesink->task);

    g_mutex_clear (&basesink->lock);
    g_cond_clear (&basesink->cond);

    gst_buffer_list_unref (basesink->bit_list);
    gst_buffer_list_unref (basesink->txt_list);
    //g_mutex_clear(&basesink->bit_lock);
    //g_mutex_clear(&basesink->txt_lock);

    if(basesink->wss_uri)
        g_free (basesink->wss_uri);
    basesink->wss_uri=NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
gst_ws_sink_get_type (void)
{
  static GType ws_sink_type = 0;

  if (!ws_sink_type) {
    static const GTypeInfo base_sink_info = {
      sizeof (GstWsSinkClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_ws_sink_class_init,
      NULL,
      NULL,
      sizeof (GstWsSink),
      0,
      (GInstanceInitFunc) gst_ws_sink_init,
      NULL
    };

    ws_sink_type =
        g_type_register_static (GST_TYPE_ELEMENT, "GstWsSink",
        &base_sink_info, 0);

    GST_DEBUG_CATEGORY_INIT (gst_ws_sink_debug, "wssink", 0,
        "Inference data sender based on WebSocket");
  }

  return ws_sink_type;
}

