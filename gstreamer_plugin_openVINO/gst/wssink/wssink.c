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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

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

/* BaseSink properties */
enum
{
  PROP_0,
  PROP_LAST
};

static const char gst_ws_bit_src_caps_str[] = "image/jpeg";
static GstStaticPadTemplate gst_ws_bit_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_bit",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (gst_ws_bit_src_caps_str));

static GstStaticPadTemplate gst_ws_txt_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_txt",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstElementClass *parent_class = NULL;

static void gst_ws_sink_get_times (GstWsSink * basesink, GstBuffer * buffer, GstClockTime * start);

#define INVALID_BUFFER_TS (-1)
// get the bit buffer and txt buffer with the same ts
// if not, find the earliest buffer as the output
static void get_buffers_with_same_ts(GstWsSink * basesink, GstBuffer **bit_buf, GstBuffer **txt_buf)
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
        return;
    }

    // bit buffer is early, only return it
    if(G_UNLIKELY(bit_ts < txt_ts)){
        GST_WS_SINK_BIT_LOCK(basesink);
        gst_buffer_list_remove(basesink->bit_list, 0, 1);
        GST_WS_SINK_BIT_UNLOCK(basesink);
        priv->bit_processed_num++;
        *txt_buf = NULL;
        *bit_buf = bit_buf_temp;
        return;
    }

    // txt buffer is early, only return it
    if(G_UNLIKELY(txt_ts < bit_ts)){
        GST_WS_SINK_TXT_LOCK(basesink);
        gst_buffer_list_remove(basesink->txt_list, 0, 1);
        GST_WS_SINK_TXT_UNLOCK(basesink);
        priv->txt_processed_num++;
        *txt_buf = txt_buf_temp;
        *bit_buf = NULL;
        return;
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

    return;
}
// Main function to processed buffer into next filter
//      1. package jpeg bitstream and inference result data
//      2. send out by WebSocket
static void process_sink_buffers(gpointer userData)
{
    GstWsSink *basesink;
    basesink = GST_WS_SINK (userData);
    GstBuffer *bit_buf = NULL, *txt_buf = NULL;
    //GstWsSinkPrivate *priv = basesink->priv;

    get_buffers_with_same_ts(basesink, &bit_buf, &txt_buf);
    if(!bit_buf && !txt_buf){
        GST_INFO("Not buffer, do nothing!");
        return;
    }

    // TODO: parse gstbuffer and package them

    // release buffer
    if(bit_buf)
        gst_buffer_unref(bit_buf);
    if(txt_buf)
        gst_buffer_unref(txt_buf);
}

static GstCaps *
gst_ws_sink_query_caps (GstWsSink * bsink, GstPad * pad, GstCaps * filter)
{
  GstCaps *caps = NULL;
  gboolean fixed;

  fixed = GST_PAD_IS_FIXED_CAPS (pad);
  if (fixed ) {
    caps = gst_pad_get_current_caps (pad);
  }

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
  //GstWsSink *sink = GST_WS_SINK (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ws_sink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  //GstWsSink *sink = GST_WS_SINK (object);

  switch (prop_id) {
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
  //GstWsSinkClass *bclass;

  basesink = GST_WS_SINK_CAST (parent);
  //bclass = GST_WS_SINK_GET_CLASS (basesink);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      GST_DEBUG_OBJECT (basesink, "caps %p", event);

      // we will not replace the caps
      gst_event_parse_caps (event, &caps);
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
    default:
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
    priv->bit_received_num++;
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
  gboolean res;
  GstWsSink *basesink;
  //GstWsSinkClass *bclass;

  basesink = GST_WS_SINK_CAST (parent);
  //bclass = GST_WS_SINK_GET_CLASS (basesink);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:
    {
      gst_ws_sink_propose_allocation (basesink, query);
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
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
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
      "Send inference data(jpeg and parameters) based on WebSock",
      "River,Li <river.li@intel.com>");

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

    gst_task_stop(basesink->task);
    gst_task_join(basesink->task);
    gst_object_unref(basesink->task);

    g_mutex_clear (&basesink->lock);
    g_cond_clear (&basesink->cond);

    gst_buffer_list_unref (basesink->bit_list);
    gst_buffer_list_unref (basesink->txt_list);
    //g_mutex_clear(&basesink->bit_lock);
    //g_mutex_clear(&basesink->txt_lock);

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

static gboolean plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "wssink", GST_RANK_PRIMARY,
          GST_TYPE_WS_SINK))
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
#define PACKAGE_NAME "GStreamer-WSSINK"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://www.intel.com"
#endif


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    wssink,
    "Send to data based on WebSocked",
    plugin_init, VERSION,
    "BSD", PACKAGE_NAME, GST_PACKAGE_ORIGIN);
