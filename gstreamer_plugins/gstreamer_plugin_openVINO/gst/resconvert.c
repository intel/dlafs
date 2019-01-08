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
#include "config.h"
#endif

#include <string.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>
#include <gst/base/gstbytereader.h>
#include "resconvert.h"
#include "respool.h"
#include <interface/videodefs.h>
#include <ocl/metadata.h>

#include <safe_mem_lib.h>
#include <safe_str_lib.h>

GST_DEBUG_CATEGORY_STATIC (resconvert_debug);
#define GST_CAT_DEFAULT (resconvert_debug)

static GstElementClass *parent_class = NULL;
//G_DEFINE_TYPE (ResConvert, res_convert, TYPE_RES_CONVERT);

/* resconvert signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  /* FILL ME for more properties*/
};

enum
{
    STREAM_TYPE_TXT = 0,
    STREAM_TYPE_PIC = 1,
};

struct _ResOclBlendPrivate
{
    // TODO: Put more private data here
    int id;
};


#define RES_CONVERT_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_RES_CONVERT, ResOclBlendPrivate))


// SINK pad caps - the same with mfxdec src pad caps
static const char sink_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("NV12") "; " \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:MFXSurface", "{ NV12 }");

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE (
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (sink_caps_str)
    );

// PIC_SRC pad caps - which should support by mfxjpegenc, only NV12 and BGRA
//  note: BGRx is not supported by mfxjpegenc
static const char src_pic_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("{BGRA}") "; " \
    GST_VIDEO_CAPS_MAKE ("NV12") "; ";

static GstStaticPadTemplate src_pic_factory =
    GST_STATIC_PAD_TEMPLATE (
    "src_pic",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS (src_pic_caps_str)
    );

// TXT_SRC pad caps
static const char src_txt_caps_str[] = "data/res_txt";
static GstStaticPadTemplate src_txt_factory =
GST_STATIC_PAD_TEMPLATE (
    "src_txt",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS (src_txt_caps_str)
    /*GST_STATIC_CAPS_ANY*/
    );

/*static guint res_convert_signals[LAST_SIGNAL] = { 0 };*/
extern VASurfaceID gst_get_mfx_surface(GstBuffer* inbuf, GstVideoInfo *info, VADisplay *display);

static GstFlowReturn
res_convert_fill_txt_data(ResMemory *res_mem, CvdlMeta *cvdl_meta)
{
    InferenceData *dst ;
    InferenceMeta *src = cvdl_meta->inference_result;
    int count = cvdl_meta->meta_count;
    size_t size = 0;

    if(!src || !count)
        return GST_FLOW_OK;

    // count out the number of object inference and resize dat memory
    res_memory_data_resize((GstMemory *)res_mem,count);

    if(!res_mem->data)
        return GST_FLOW_OK;
 
    dst = res_mem->data; 
    for(int i=0; i<count; i++) {
        dst[i].rect = src->rect;
        dst[i].probility = src->probility;
        strcpy_s(dst[i].label,128, src->label);
        size = src->track_count*sizeof(VideoPoint);
        memcpy_s(dst[i].track, size, src->track, size);
        dst[i].track_num = src->track_count;
        src = src->next;
    }
    return GST_FLOW_OK;
}

static GstFlowReturn
res_convert_send_data (ResConvert * convertor, GstBuffer * buf, GstBuffer * inbuf)
{
    GstFlowReturn result = GST_FLOW_OK;
    GstBuffer *txt_buf;
    ResMemory *txt_mem;
    CvdlMeta *cvdl_meta;
    
    GST_LOG_OBJECT (convertor, "pushing buffer...");

    // For txt data: 1) allocate buffer,  2) fill buffer, 3) push it
    cvdl_meta = gst_buffer_get_cvdl_meta (inbuf);
    if(convertor->txt_srcpad && cvdl_meta){
        txt_buf = res_buffer_alloc(convertor->src_pool);
        if(txt_buf){
            txt_mem = RES_MEMORY_CAST(res_memory_acquire(txt_buf));
            res_convert_fill_txt_data(txt_mem, cvdl_meta);
            txt_mem->pts = GST_BUFFER_PTS (inbuf);
            GST_BUFFER_PTS(txt_buf) = GST_BUFFER_PTS (inbuf);
            GST_BUFFER_DTS(txt_buf) = GST_BUFFER_DTS (inbuf);
            GST_BUFFER_DURATION(txt_buf) = GST_BUFFER_DURATION (inbuf);
            gst_pad_push (convertor->txt_srcpad, txt_buf);
        }
    }

    void *data = NULL;
    GST_DEBUG("resconvert out: inbuf = %p(%d), buffer = %p(%d), surface = %d\n",
        inbuf, GST_MINI_OBJECT_REFCOUNT(inbuf),
        buf, GST_MINI_OBJECT_REFCOUNT(buf),
        gst_get_mfx_surface(inbuf, NULL, &data));

    // For pic data which has been blended already:
    //  1) push the buffer directly
    if(convertor->pic_srcpad){
        // gst_pad_push will unref this buffer.
        gst_pad_push (convertor->pic_srcpad, buf);
    } else {
        gst_buffer_unref(buf);
    }

    GST_LOG_OBJECT (convertor, "result: %s", gst_flow_get_name (result));

    return result;
}


static gboolean
res_convert_send_event (ResConvert * convertor, GstEvent * event)
{
  gboolean ret = TRUE;

  if (convertor->pic_srcpad && !gst_pad_push_event (convertor->pic_srcpad, gst_event_ref (event))) {
       GST_DEBUG_OBJECT (convertor->pic_srcpad, "%s event was not handled by pic_pad",
            GST_EVENT_TYPE_NAME (event));
       ret = FALSE;
  } else {
        /* If at least one push returns TRUE, then we return TRUE. */
        GST_DEBUG_OBJECT (convertor->pic_srcpad, "%s event was handled",
            GST_EVENT_TYPE_NAME (event));
  }

  if (convertor->txt_srcpad && !gst_pad_push_event (convertor->txt_srcpad, gst_event_ref (event))) {
       GST_DEBUG_OBJECT (convertor->txt_srcpad, "%s event was not handled by txt_pad",
            GST_EVENT_TYPE_NAME (event));
       ret = FALSE;
  } else {
        /* If at least one push returns TRUE, then we return TRUE. */
        GST_DEBUG_OBJECT (convertor->txt_srcpad, "%s event was handled",
            GST_EVENT_TYPE_NAME (event));
  }

  gst_event_unref (event);
  return ret;
}


static void
res_convert_flush (ResConvert * convertor)
{
  GST_DEBUG_OBJECT (convertor, "flushing convertorer");
  //TODO
}


static void
res_convert_clean (ResConvert * convertor)
{
    // g_object_unref() --> gst_element_dispose
    // it has unref all the pads

    // release pool
    if(convertor->src_pool) {
        gst_object_unref (convertor->src_pool);
        convertor->src_pool = NULL;
    }
    if(convertor->blend_handle)
        blender_destroy(convertor->blend_handle);
    convertor->blend_handle = 0;
}

static void
res_convert_finalize (ResConvert * convertor)
{
    // g_object_unref() --> gst_element_dispose
    // it has unref all the pads

    // release pool
    res_convert_clean(convertor);

    G_OBJECT_CLASS (parent_class)->finalize (G_OBJECT (convertor));
}


static gboolean
res_convert_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
    gboolean res = TRUE;
    ResConvert *convertor = RES_CONVERT (parent);
    GstCaps *caps = NULL;

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
        res_convert_send_event (convertor, event);
        break;
    case GST_EVENT_FLUSH_STOP:
        res_convert_send_event (convertor, event);
        res_convert_flush (convertor);
        res_convert_clean(convertor);
        break;
    case GST_EVENT_SEGMENT:
    {
        gst_event_unref (event);
        break;
    }
    case GST_EVENT_EOS:
        GST_INFO_OBJECT (convertor, "Received EOS");
        if (!res_convert_send_event (convertor, event)) {
            GST_WARNING_OBJECT (convertor, "EOS and no streams open");
        }
        break;
    case GST_EVENT_CAPS:
        gst_event_parse_caps (event, &caps);

        gchar *log = gst_caps_to_string(caps);
        GST_INFO("event_caps =\n %s\n", log);
        g_free(log);
        log = NULL;

        // get info of caps
        gst_video_info_from_caps (&convertor->sink_info, caps);

        /**---  set caps here  --*/
        // From src pad event, only process sink pad caps
        // From sink pad event, only process src pad caps
        // If not, there will be dead-loop
        //
        // For example: event from sink pad, then call below function will be dead lock
        //              gst_pad_set_caps (convertor->sinkpad, caps);

        /**---set caps for peer pad below----*/
        GstPadDirection direction = GST_PAD_DIRECTION (pad);
        GstCaps *prev_incaps = NULL, *newcaps;
        //GstPad *mypad = NULL;
        GstPad *otherpad = NULL;
        if(direction==GST_PAD_SRC){
            //mypad = convertor->pic_srcpad;
            otherpad  = convertor->sinkpad;
        }else{
            //mypad = convertor->sinkpad;
            otherpad = convertor->pic_srcpad;
        }

        // Don't process src event
        if(direction==GST_PAD_SRC){
            // TODO: how to process src event?
            gst_event_unref (event);
            break;
        }

        // set pic_src pad
        prev_incaps = gst_pad_get_current_caps (otherpad);
        GST_DEBUG("caps = %s\n", gst_caps_to_string(caps));
        GST_DEBUG("1.prev_incaps = %s\n", gst_caps_to_string(prev_incaps));
        if(!prev_incaps)
            prev_incaps = gst_pad_get_pad_template_caps(otherpad);
        GST_DEBUG("2.prev_incaps = %s\n", gst_caps_to_string(prev_incaps));

        // create a new caps based on input caps of event
        GstStructure *structure;
        GstCapsFeatures *features;

        newcaps = gst_caps_new_empty();
        GST_CAPS_FLAGS (newcaps) = GST_CAPS_FLAGS (caps);

        structure  = gst_structure_copy(gst_caps_get_structure(caps, 0));
        //features = gst_caps_features_copy(gst_caps_get_features(caps, 0));
        GST_DEBUG("structure:\n%s\n",gst_structure_to_string(structure));
        //g_printf("features:\n%s\n",gst_caps_features_to_string(features));

        gst_structure_set_name(structure, "video/x-raw");
        gst_structure_remove_field(structure, "format=(string)NV12");
        gst_structure_set(structure,"format",G_TYPE_STRING, "BGRA", NULL);
        GST_DEBUG("structure:\n%s\n",gst_structure_to_string(structure));
        features = gst_caps_features_new_empty();

        gst_caps_append_structure_full(newcaps, structure, features);
        GST_DEBUG("caps =\n %s\n", gst_caps_to_string(caps));
        GST_DEBUG("newcaps =\n %s\n", gst_caps_to_string(newcaps));
        newcaps = gst_caps_intersect_full (newcaps, prev_incaps, GST_CAPS_INTERSECT_FIRST);
        //g_print("intersect newcaps =\n %s\n", gst_caps_to_string(newcaps));

        // TODO: need to free structure and features?

        // set caps
        gst_pad_set_caps (otherpad, newcaps);
        blender_init(convertor->blend_handle ,caps);

        // push this caps to next filter
        GstEvent *new_event = gst_event_new_caps(newcaps);
        res = gst_pad_push_event (convertor->pic_srcpad, gst_event_ref(new_event));
        gst_event_unref (new_event);

        // free caps
        gst_caps_unref(newcaps);
        gst_caps_unref (prev_incaps);

        // TODO: set caps for txt_srcpad

        // free event
        gst_event_unref (event);
        break;
    default:
        res = res_convert_send_event (convertor, event);
        break;
    }

  return res;
}


static gboolean
res_convert_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = FALSE;
  ResConvert *convertor = RES_CONVERT (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_RECONFIGURE:
        break;
    default:
      res = gst_pad_push_event (convertor->sinkpad, event);
      break;
  }

  return res;
}

static gboolean
res_convert_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
    ResConvert *convertor = RES_CONVERT (parent);
    //GstPadDirection direction = GST_PAD_DIRECTION (pad);
    gboolean ret = TRUE;//, forward = FALSE;

    GST_LOG_OBJECT (convertor, "Have query of type %d on pad %" GST_PTR_FORMAT,
        GST_QUERY_TYPE (query), pad);

    GST_DEBUG("%s() - name = %s, query = %s\n",__func__, gst_pad_get_name(pad),
        GST_QUERY_TYPE_NAME (query));

    ret = gst_pad_query_default (pad, parent, query);

    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_ALLOCATION:
            GST_DEBUG("%s() - query GST_QUERY_ALLOCATION...",__func__);
            break;
        case GST_QUERY_ACCEPT_CAPS:
        {
            GstCaps *caps;
            gst_query_parse_accept_caps (query, &caps);
            GST_DEBUG("%s() - accept caps = %s\n", __func__, gst_caps_to_string(caps));
        break;
        }
        case GST_QUERY_CAPS:
        {
            GstCaps *caps;
            gst_query_parse_caps (query, &caps);
            GST_DEBUG("%s() - caps = %s\n", __func__, gst_caps_to_string(caps));
            break;
        }
        default:
            break;
    }

    return ret;
}


/* If we can pull that's prefered */
static gboolean
res_convert_sink_activate (GstPad * sinkpad, GstObject * parent)
{
    gboolean res = FALSE;
    GstQuery *query = gst_query_new_formats();
    if (gst_pad_peer_query (sinkpad, query)) {
        res = gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PUSH, TRUE);
    } 
    gst_query_unref (query);
    return res;
}

/* This function gets called when we activate ourselves in push mode. */
static gboolean
res_convert_sink_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
    gboolean res = TRUE;

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

static GstFlowReturn
res_convert_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
    ResConvert *convertor = RES_CONVERT (parent);
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer* output = NULL;
    gint64 start, stop;

    GST_LOG_OBJECT (convertor,
        "Received buffer with offset %" G_GUINT64_FORMAT,
        GST_BUFFER_OFFSET (buffer));
    start = g_get_monotonic_time();

    // Process the input buffer - draw data into NV12 surface by OpenCV
    // The src_pic will connect to a queue filter to cache the data
    // step 1: create a RGB OSD and draw string/rectangle/track points on it
    // step 2: call OpenCL to blend OSD with NV12 surface, into OSD buffer?
    output = blender_process_cvdl_buffer(convertor->blend_handle, buffer);
    convertor->frame_num++;

    stop = g_get_monotonic_time();
    convertor->cost_ms = (stop-start)/1000;
    // push the result
    if(output)
        ret = res_convert_send_data (convertor, output, buffer);
    gst_buffer_unref(buffer);

    return ret;
}

static gboolean
res_convert_create_src_pad (ResConvert * convertor, gint stream_type)
{
    ResConvertClass *klass = RES_CONVERT_GET_CLASS (convertor);
    GST_DEBUG_OBJECT (convertor, "create src pad, type: 0x%02x", stream_type);

    switch (stream_type) {
        case STREAM_TYPE_TXT:
        {
            convertor->txt_srcpad =
                gst_pad_new_from_template (klass->src_txt_template, "src_txt");
            gst_pad_set_event_function (convertor->txt_srcpad,
                GST_DEBUG_FUNCPTR (res_convert_src_event));
            gst_pad_set_query_function (convertor->txt_srcpad,
                GST_DEBUG_FUNCPTR (res_convert_query));
            gst_pad_use_fixed_caps (convertor->txt_srcpad);
            if (!gst_pad_set_active (convertor->txt_srcpad, TRUE)) {
                GST_WARNING_OBJECT (convertor, "Failed to activate pad %" GST_PTR_FORMAT,
                    convertor->txt_srcpad);
                return FALSE;
            }
            gst_element_add_pad (GST_ELEMENT (convertor), convertor->txt_srcpad);
            break;
        }
        case STREAM_TYPE_PIC:
        {
            convertor->pic_srcpad =
                gst_pad_new_from_template (klass->src_pic_template, "src_pic");
            gst_pad_set_event_function (convertor->pic_srcpad,
                GST_DEBUG_FUNCPTR (res_convert_src_event));
            gst_pad_set_query_function (convertor->pic_srcpad,
                GST_DEBUG_FUNCPTR (res_convert_query));
            gst_pad_use_fixed_caps (convertor->pic_srcpad);
            if (!gst_pad_set_active (convertor->pic_srcpad, TRUE)) {
                GST_WARNING_OBJECT (convertor, "Failed to activate pad %" GST_PTR_FORMAT,
                    convertor->pic_srcpad);
                return FALSE;
            }
            gst_element_add_pad (GST_ELEMENT (convertor), convertor->pic_srcpad);
            break;
        }
        default:
            break;
    }

    return TRUE;
}


static void
res_convert_reset (ResConvert * convertor)
{
    // It should unref all the pads
    // but g_object_unref() --> gst_element_dispose has done it
}

static GstStateChangeReturn
res_convert_change_state (GstElement * element, GstStateChange transition)
{
    ResConvert *convertor = RES_CONVERT (element);
    GstStateChangeReturn result;
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            // create the process task(thread) and start it

            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            break;
        default:
            break;
    }

  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      res_convert_reset (convertor);
      g_print("resconvert processed %ld frames, fps = %.2f\n", convertor->frame_num,
          1000.0 * convertor->frame_num / (0.01 + convertor->cost_ms));
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      // stop the thread
      res_convert_clean(convertor);
      break;
    default:
      break;
  }

  return result;
}


static void
res_convert_base_init (ResConvertClass * klass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

    klass->sink_template    = gst_static_pad_template_get (&sink_factory);
    klass->src_pic_template = gst_static_pad_template_get (&src_pic_factory);
    klass->src_txt_template = gst_static_pad_template_get (&src_txt_factory);

    gst_element_class_add_pad_template (element_class, klass->src_pic_template);
    gst_element_class_add_pad_template (element_class, klass->src_txt_template);
    gst_element_class_add_pad_template (element_class, klass->sink_template);

    gst_element_class_set_static_metadata (element_class,
      "Inference result convertor", "Transform/PostProcess",
      "Convert inference result into OSD", "River Li<river.li@intel.com>");
}

static void
res_convert_class_init (ResConvertClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->finalize = (GObjectFinalizeFunc) res_convert_finalize;
    gstelement_class->change_state = res_convert_change_state;

    g_type_class_add_private (klass, sizeof (ResOclBlendPrivate));
}

static void
res_convert_init (ResConvert * convertor)
{
    ResConvertClass *klass = RES_CONVERT_GET_CLASS (convertor);
    //GstCaps *caps = gst_caps_new_any();

    GST_DEBUG_CATEGORY_INIT (resconvert_debug, "resconvert", 0,
            "Convert inference result into OSD");

    //TODO:
    GstCaps *caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGRA", NULL);
    gst_caps_set_simple (caps, "width", G_TYPE_INT, sizeof(InferenceData), "height",
      G_TYPE_INT, 1, NULL);

    /* Create sink pad */
    convertor->sinkpad = gst_pad_new_from_template (klass->sink_template, "sink");
    gst_pad_set_event_function (convertor->sinkpad, GST_DEBUG_FUNCPTR (res_convert_sink_event));
    gst_pad_set_query_function (convertor->sinkpad, GST_DEBUG_FUNCPTR (res_convert_query));
    gst_pad_set_chain_function (convertor->sinkpad, GST_DEBUG_FUNCPTR (res_convert_chain));
    gst_pad_set_activate_function (convertor->sinkpad, GST_DEBUG_FUNCPTR (res_convert_sink_activate));
    gst_pad_set_activatemode_function (convertor->sinkpad,  GST_DEBUG_FUNCPTR (res_convert_sink_activate_mode));
    gst_element_add_pad (GST_ELEMENT (convertor), convertor->sinkpad);

    /* create a pool for src_txt buffer */
    convertor->src_pool = res_pool_create (caps, sizeof(InferenceData), 3, 10);
    gst_caps_unref(caps);

    /* create src pads */
    //TODO: also we can create src pads dynamically in future
    res_convert_create_src_pad (convertor, STREAM_TYPE_TXT);
    res_convert_create_src_pad (convertor, STREAM_TYPE_PIC);

    convertor->priv = RES_CONVERT_GET_PRIVATE (convertor);
    gst_video_info_init (&convertor->sink_info);
    gst_video_info_init (&convertor->src_info);

    convertor->blend_handle = blender_create();
    convertor->cost_ms = 0;
    convertor->frame_num = 0;
}


GType
res_convert_get_type (void)
{
  static GType res_convert_type = 0;

  if (!res_convert_type) {
    static const GTypeInfo res_convert_info = {
      sizeof (ResConvertClass),
      (GBaseInitFunc) res_convert_base_init,
      NULL,
      (GClassInitFunc) res_convert_class_init,
      NULL,
      NULL,
      sizeof (ResConvert),
      0,
      (GInstanceInitFunc) res_convert_init,
      NULL
    };

    res_convert_type =
        g_type_register_static (GST_TYPE_ELEMENT, "ResConvert",
        &res_convert_info, 0);

    GST_DEBUG_CATEGORY_INIT (resconvert_debug, "resconvert", 0,
        "Inference result convertor element");
  }

  return res_convert_type;
}

