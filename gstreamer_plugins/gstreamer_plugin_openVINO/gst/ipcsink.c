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


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <interface/videodefs.h>
#include "resmemory.h"
#include "ipcsink.h"


GST_DEBUG_CATEGORY_STATIC (gst_ipc_sink_debug);
#define GST_CAT_DEFAULT gst_ipc_sink_debug

#define GST_IPC_SINK_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_IPC_SINK, GstIpcSinkPrivate))

struct _GstIpcSinkPrivate
{
    /* number of received and processed buffers */
    guint64 bit_received_num;
    guint64 bit_processed_num;
    guint64 txt_received_num;
    guint64 txt_processed_num;
};

/* IpcSink properties */
enum
{
    PROP_0,
    PROP_IPC_SERVER_URI,
    PROP_IPC_CLIENT_ID,
    PROP_IPC_CLIENT_PROXY,
    PROP_LAST
};

#define INVALID_IPCC_PROXY  ((void *)(-1))
#define TXT_BUFFER_SIZE_MAX 1024

static const char gst_ipc_bit_src_caps_str[] = "image/jpeg";
static GstStaticPadTemplate gst_ipc_bit_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_bit",
    GST_PAD_SINK,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS (gst_ipc_bit_src_caps_str));

static const char gst_ipc_txt_src_caps_str[] = "data/res_txt";
static GstStaticPadTemplate gst_ipc_txt_src_factory =
GST_STATIC_PAD_TEMPLATE ("sink_txt",
    GST_PAD_SINK,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS (gst_ipc_txt_src_caps_str));
    /*GST_STATIC_CAPS_ANY);*/

static GstElementClass *parent_class = NULL;

static void gst_ipc_sink_get_times (GstIpcSink * basesink, GstBuffer * buffer, GstClockTime * start);

#define INVALID_BUFFER_TS (-1)
// get the bit buffer and txt buffer with the same ts
// if not, find the earliest buffer as the output
static gboolean get_buffers_with_same_ts(GstIpcSink * basesink, GstBuffer **bit_buf, GstBuffer **txt_buf)
{
    GstBuffer *bit_buf_temp, *txt_buf_temp;
    GstClockTime bit_ts=INVALID_BUFFER_TS;
    GstClockTime txt_ts=INVALID_BUFFER_TS;
    GstIpcSinkPrivate *priv = basesink->priv;

    txt_buf_temp = NULL;
    bit_buf_temp = NULL;

    // try to get a buffer of bitstream
    GST_IPC_SINK_BIT_LOCK(basesink);
    if(gst_buffer_list_length(basesink->bit_list)>0) {
        bit_buf_temp = gst_buffer_list_get(basesink->bit_list,0);
        gst_ipc_sink_get_times(basesink, bit_buf_temp, &bit_ts);
    }
    GST_IPC_SINK_BIT_UNLOCK(basesink);

    // try to get a buffer of txt data
    GST_IPC_SINK_TXT_LOCK(basesink);
    if(gst_buffer_list_length(basesink->txt_list)>0) {
        txt_buf_temp = gst_buffer_list_get(basesink->txt_list,0);
        gst_ipc_sink_get_times(basesink, txt_buf_temp, &txt_ts);
    }
    GST_IPC_SINK_TXT_UNLOCK(basesink);

    // Improvement need: align bit buffer and txt buffer
    // can not get any, return NULL
    if(G_UNLIKELY((!bit_buf_temp && !txt_buf_temp))) {
        *txt_buf = NULL;
        *bit_buf = NULL;
        return FALSE;
    }

    // bit buffer is early, only return it
    if(G_UNLIKELY(bit_ts < txt_ts)){
        GST_IPC_SINK_BIT_LOCK(basesink);
        gst_buffer_list_remove(basesink->bit_list, 0, 1);
        GST_IPC_SINK_BIT_UNLOCK(basesink);
        priv->bit_processed_num++;
        *txt_buf = NULL;
        *bit_buf = bit_buf_temp;
        return TRUE;
    }

    // txt buffer is early, only return it
    if(G_UNLIKELY(txt_ts < bit_ts)){
        GST_IPC_SINK_TXT_LOCK(basesink);
        gst_buffer_list_remove(basesink->txt_list, 0, 1);
        GST_IPC_SINK_TXT_UNLOCK(basesink);
        priv->txt_processed_num++;
        *txt_buf = txt_buf_temp;
        *bit_buf = NULL;
        return TRUE;
    }

    if(bit_buf_temp) {
        GST_IPC_SINK_BIT_LOCK(basesink);
        gst_buffer_list_remove(basesink->bit_list, 0, 1);
        GST_IPC_SINK_BIT_UNLOCK(basesink);
        priv->bit_processed_num++;
    }

    if(txt_buf_temp) {
        GST_IPC_SINK_TXT_LOCK(basesink);
        gst_buffer_list_remove(basesink->txt_list, 0, 1);
        GST_IPC_SINK_TXT_UNLOCK(basesink);
        priv->txt_processed_num++;
    }

    *txt_buf = txt_buf_temp;
    *bit_buf = bit_buf_temp;

    return TRUE;
}


static gboolean data_list_is_empty(GstIpcSink * basesink)
{
    int num = 0;
    // try to get a buffer of bitstream
    GST_IPC_SINK_BIT_LOCK(basesink);
    num = gst_buffer_list_length(basesink->bit_list);
    GST_IPC_SINK_BIT_UNLOCK(basesink);

    // try to get a buffer of txt data
    GST_IPC_SINK_TXT_LOCK(basesink);
    num += gst_buffer_list_length(basesink->txt_list);
    GST_IPC_SINK_TXT_UNLOCK(basesink);

    return num<=0;
}
// Main function to processed buffer into next filter
//      1. package jpeg bitstream and inference result data
//      2. send out by WebSocket
static void process_sink_buffers(gpointer userData)
{
    GstIpcSink *basesink;
    basesink = GST_IPC_SINK (userData);
    GstBuffer *bit_buf = NULL, *txt_buf = NULL;
    gboolean ret = FALSE;
    int i = 0;
    gsize data_len = 0, size = 0;
    //GstIpcSinkPrivate *priv = basesink->priv;

    ret = get_buffers_with_same_ts(basesink, &bit_buf, &txt_buf);
    if(ret==FALSE) {
        g_usleep(10000);
        return;
    }

    //setup ipcclient
    if(!basesink->ipc_handle) {
        if(basesink->ipc_handle_proxy != INVALID_IPCC_PROXY)
               basesink->ipc_handle = basesink->ipc_handle_proxy;
        else
                basesink->ipc_handle = ipcclient_setup(basesink->ipcs_uri, basesink->ipcc_id);
        ipcclient_set_id(basesink->ipc_handle,  basesink->ipcc_id);
    }

    // meta data
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
            #if 0
            for(i=0; i<count; i++) {
                g_print("pipe %d: %d frame, %2d xml_data - ",basesink->ipcc_id,
                    basesink->frame_index, basesink->meta_data_index);
                infer_data->frame_index = basesink->frame_index;
                data_len = ipcclient_send_infer_data(basesink->ipc_handle,
                    infer_data, txt_mem->pts, basesink->meta_data_index);
                basesink->meta_data_index++;
                size += data_len;
                infer_data++;
            }
            #else
            infer_data->frame_index = basesink->frame_index;
            data_len = ipcclient_send_infer_data_full_frame(basesink->ipc_handle,
                infer_data, count,txt_mem->pts,basesink->meta_data_index);
            basesink->meta_data_index+=count;
            size += data_len;
            #endif
            if(count>0)
                basesink->frame_index++;
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
                ipcclient_send_data(basesink->ipc_handle, (const char *)data_base, data_len, eMetaJPG);
                g_print("pipe %d: index = %d, jpeg size = %ld\n",basesink->ipcc_id, basesink->bit_data_index, data_len );
                basesink->bit_data_index++;
            }
            size += data_len;
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
gst_ipc_sink_query_caps (GstIpcSink * bsink, GstPad * pad, GstCaps * filter)
{
    GstCaps *caps = NULL, *temp=NULL;
    GstPadDirection direction = GST_PAD_DIRECTION (pad);

    if(direction==GST_PAD_SRC) {
        GST_ERROR("It has no src pad!!!\n");
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
gst_ipc_sink_fixate (GstIpcSink * bsink, GstCaps * caps)
{
    GST_DEBUG_OBJECT (bsink, "using default caps fixate function");

    caps = gst_caps_fixate (caps);
    return caps;
}

static gboolean
gst_ipc_sink_propose_allocation (GstIpcSink *sink, GstQuery *query)
{
    /* propose allocation parameters for upstream */
    return FALSE;
}

static void
gst_ipc_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstIpcSink *sink = GST_IPC_SINK (object);

  switch (prop_id) {
    case PROP_IPC_SERVER_URI:
        if(sink->ipcs_uri)
            g_free(sink->ipcs_uri);
        sink->ipcs_uri = g_value_dup_string (value);
        break;
    case PROP_IPC_CLIENT_ID:
        sink->ipcc_id = g_value_get_int(value);
        break;
     case PROP_IPC_CLIENT_PROXY:
        sink->ipc_handle_proxy=g_value_get_pointer(value);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ipc_sink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstIpcSink *sink = GST_IPC_SINK (object);

  switch (prop_id) {
    case PROP_IPC_SERVER_URI:
        g_value_set_string (value, sink->ipcs_uri);
        break;
    case PROP_IPC_CLIENT_ID:
        g_value_set_int (value, sink->ipcc_id);
        break;
    case PROP_IPC_CLIENT_PROXY:
        g_value_set_pointer(value, sink->ipc_handle_proxy);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_ipc_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstIpcSink *basesink;
  gboolean result = TRUE;

  basesink = GST_IPC_SINK_CAST (parent);
  //bclass = GST_IPC_SINK_GET_CLASS (basesink);

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
static void gst_ipc_sink_get_times (GstIpcSink * basesink, GstBuffer * buffer, GstClockTime * start)
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
gst_ipc_sink_bit_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstIpcSink *basesink;
    basesink = GST_IPC_SINK (parent);
    GstIpcSinkPrivate *priv = basesink->priv;

    // put buffer into bufferlist
    GST_IPC_SINK_BIT_LOCK(parent);
    buf = gst_buffer_ref(buf);
    gst_buffer_list_add(basesink->bit_list, buf);
    priv->bit_received_num++;
    GST_IPC_SINK_BIT_UNLOCK(parent);

    return GST_FLOW_OK;
}

static GstFlowReturn
gst_ipc_sink_txt_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstIpcSink *basesink;
    basesink = GST_IPC_SINK (parent);
    GstIpcSinkPrivate *priv = basesink->priv;

    // put buffer into bufferlist
    GST_IPC_SINK_TXT_LOCK(parent);
    buf = gst_buffer_ref(buf);
    gst_buffer_list_add(basesink->txt_list, buf);
    priv->txt_received_num++;
    GST_IPC_SINK_TXT_UNLOCK(parent);

    return GST_FLOW_OK;
}


static gboolean
gst_ipc_sink_pad_activate (GstPad * pad, GstObject * parent)
{
  gboolean result = FALSE;
  GstIpcSink *basesink;

  basesink = GST_IPC_SINK (parent);

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
gst_ipc_sink_pad_activate_mode (GstPad * pad, GstObject * parent,
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
gst_ipc_sink_send_event (GstElement * element, GstEvent * event)
{
  GstIpcSink *basesink = GST_IPC_SINK (element);
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
  GstIpcSink *basesink = GST_IPC_SINK (element);

  // Improvement need: how to query the 2 sink pad?
  res = gst_pad_peer_query (basesink->sinkpad_bit, query);
  GST_DEBUG_OBJECT (basesink, "query %s returns %d",
      GST_QUERY_TYPE_NAME (query), res);
  return res;
}

static gboolean
//gst_ipc_sink_default_query (GstIpcSink * basesink, GstPad *pad, GstQuery * query)
gst_ipc_sink_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  GstIpcSink *basesink;
  //GstIpcSinkClass *bclass;

  basesink = GST_IPC_SINK_CAST (parent);
  //bclass = GST_IPC_SINK_GET_CLASS (basesink);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:
    {
      res = gst_ipc_sink_propose_allocation (basesink, query);
      break;
    }
    case GST_QUERY_CAPS:
    {
      GstCaps *caps, *filter;

      gst_query_parse_caps (query, &filter);
      caps = gst_ipc_sink_query_caps (basesink, pad, filter);
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
      allowed = gst_ipc_sink_query_caps (basesink, pad, NULL);
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
gst_ipc_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstIpcSink *basesink = GST_IPC_SINK (element);
  //GstIpcSinkClass *bclass;
  //bclass = GST_IPC_SINK_GET_CLASS (basesink);

  switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            GST_DEBUG_OBJECT (basesink, "NULL to READY");
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            GST_DEBUG_OBJECT (basesink, "READY to PAUSED");
            break;
         case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            GST_DEBUG_OBJECT (basesink, "PAUSED to PLAYING, don't need preroll");

             // create task for push data to next filter/element
            basesink->task = gst_task_new (process_sink_buffers, (gpointer)basesink, NULL);
            gst_task_set_lock (basesink->task, &basesink->task_lock);
            gst_task_set_enter_callback (basesink->task, NULL, NULL, NULL);
            gst_task_set_leave_callback (basesink->task, NULL, NULL, NULL);

            // start task
            gst_task_start(basesink->task);
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
            g_print("IPCSink: PLAYING to PAUSED\n");
            int times = 20;
            while(!data_list_is_empty(basesink) && times-->0) {
                g_usleep(10000);
            }
            // stop task
            gst_task_stop(basesink->task);
            gst_task_join(basesink->task);
            gst_object_unref(basesink->task);
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

static void gst_ipc_sink_finalize (GObject * object);
static void
gst_ipc_sink_class_init (GstIpcSinkClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);

    GST_DEBUG_CATEGORY_INIT (gst_ipc_sink_debug, "basesink", 0,
        "basesink element");

    g_type_class_add_private (klass, sizeof (GstIpcSinkPrivate));
    parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = gst_ipc_sink_finalize;
    gobject_class->set_property = gst_ipc_sink_set_property;
    gobject_class->get_property = gst_ipc_sink_get_property;

    gst_element_class_set_details_simple (gstelement_class,
      "IpcSink", "Sink",
      "Send inference data(jpeg and parameters) based on IPC(socket)",
      "River,Li <river.li@intel.com>");

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IPC_SERVER_URI,
      g_param_spec_string ("ipcsuri", "IPCUri",
          "The URI of IPC(Socket) Server", "wss://localhost:8123/binaryEchoWithSize",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IPC_CLIENT_ID,
      g_param_spec_int ("ipcclientid", "IPC client Index",
          "IPC(Socket) client index to connected to its server(default: 0)",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IPC_CLIENT_PROXY,
      g_param_spec_pointer ("ipcclientproxy", "IPC client proxy",
          "IPC(Socket) client proxy to connected to its server.",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /* src pad */
    gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ipc_bit_src_factory));
    gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_ipc_txt_src_factory));

    gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_ipc_sink_change_state);
    gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_ipc_sink_send_event);
    gstelement_class->query = GST_DEBUG_FUNCPTR (default_element_query);

    /* Registering debug symbols for function pointers */
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_fixate);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_pad_activate);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_pad_activate_mode);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_event);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_bit_chain);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_txt_chain);
    GST_DEBUG_REGISTER_FUNCPTR (gst_ipc_sink_sink_query);
}


static void
gst_ipc_sink_init (GstIpcSink * basesink, gpointer g_class)
{
    GstPadTemplate *pad_template;
    GstIpcSinkPrivate *priv;

    GST_DEBUG_CATEGORY_INIT (gst_ipc_sink_debug, "ipcsink", 0,
            "Send data out by IPC/Socket");

    basesink->ipc_handle = NULL;
    basesink->ipc_handle_proxy = INVALID_IPCC_PROXY;
    basesink->ipcs_uri = NULL;
    basesink->ipcc_id = 0;
    basesink->meta_data_index = 0;
    basesink->bit_data_index = 0;
    basesink->frame_index = 0;

    basesink->priv = priv = GST_IPC_SINK_GET_PRIVATE (basesink);

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

    gst_pad_set_activate_function (basesink->sinkpad_bit, gst_ipc_sink_pad_activate);
    gst_pad_set_activatemode_function (basesink->sinkpad_bit, gst_ipc_sink_pad_activate_mode);
    gst_pad_set_query_function (basesink->sinkpad_bit, gst_ipc_sink_sink_query);
    gst_pad_set_event_function (basesink->sinkpad_bit, gst_ipc_sink_event);
    gst_pad_set_chain_function (basesink->sinkpad_bit, gst_ipc_sink_bit_chain);
    gst_element_add_pad (GST_ELEMENT_CAST (basesink), basesink->sinkpad_bit);

    gst_pad_set_activate_function (basesink->sinkpad_txt, gst_ipc_sink_pad_activate);
    gst_pad_set_activatemode_function (basesink->sinkpad_txt, gst_ipc_sink_pad_activate_mode);
    gst_pad_set_query_function (basesink->sinkpad_txt, gst_ipc_sink_sink_query);
    gst_pad_set_event_function (basesink->sinkpad_txt, gst_ipc_sink_event);
    gst_pad_set_chain_function (basesink->sinkpad_txt, gst_ipc_sink_txt_chain);
    gst_element_add_pad (GST_ELEMENT_CAST (basesink), basesink->sinkpad_txt);

    GST_OBJECT_FLAG_SET (basesink, GST_ELEMENT_FLAG_SINK);

    // create buffer list
    basesink->bit_list = gst_buffer_list_new ();
    basesink->txt_list = gst_buffer_list_new ();

    g_rec_mutex_init (&basesink->task_lock);
}

static void
gst_ipc_sink_finalize (GObject * object)
{
    GstIpcSink *basesink;
    basesink = GST_IPC_SINK (object);

   if((basesink->ipc_handle_proxy==INVALID_IPCC_PROXY)
    &&( basesink->ipc_handle) ) {
        ipcclient_destroy(basesink->ipc_handle);
        basesink->ipc_handle = NULL;
    }

    g_mutex_clear (&basesink->lock);
    g_cond_clear (&basesink->cond);

    gst_buffer_list_unref (basesink->bit_list);
    gst_buffer_list_unref (basesink->txt_list);
    //g_mutex_clear(&basesink->bit_lock);
    //g_mutex_clear(&basesink->txt_lock);

    if(basesink->ipcs_uri)
        g_free (basesink->ipcs_uri);
    basesink->ipcs_uri=NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
gst_ipc_sink_get_type (void)
{
  static GType ipc_sink_type = 0;

  if (!ipc_sink_type) {
    static const GTypeInfo base_sink_info = {
      sizeof (GstIpcSinkClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_ipc_sink_class_init,
      NULL,
      NULL,
      sizeof (GstIpcSink),
      0,
      (GInstanceInitFunc) gst_ipc_sink_init,
      NULL
    };

    ipc_sink_type =
        g_type_register_static (GST_TYPE_ELEMENT, "GstIpcSink",
        &base_sink_info, 0);

    GST_DEBUG_CATEGORY_INIT (gst_ipc_sink_debug, "ipcsink", 0,
        "Inference data sender based on IPC(Socket).");
  }

  return ipc_sink_type;
}

