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

#include "cvdlfilter.h"
#include <ocl/crcmeta.h>

// This plugin is used to apply CV/DL algothrim into video frame, and attach the result as MetaData of GstBuffer
//
// It will do all CV/DL algorithms, and the complex processing flow and intermedia buffers will be handled by itself.
// Traditional CV algorithm (CSC, Resize, object track) will be done in OpenCV/OpenCL component and
// DL algorithm (object detection/recognition) will be done in IE/HDDL-R component.
//
//Each CV/DL algorithm is encapsulated to be an algo element, in which preprocessing and inference are bind together.
//For example, object detection algorithm, is comprise of preprocessing and inference, csc+resize is preprocessing and detection is inference.

GST_DEBUG_CATEGORY_STATIC (cvdl_filter_debug);
#define GST_CAT_DEFAULT cvdl_filter_debug


#define cvdl_filter_parent_class parent_class
G_DEFINE_TYPE (CvdlFilter, cvdl_filter, GST_TYPE_BASE_TRANSFORM);

#define CVDL_FILTER_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CVDL_FILTER_TYPE, CvdlFilterPrivate))

// Whether sync decoder with cvdlfilter
// It will make decode and cvdl to be processed by the same fps
// #define SYNC_WITH_DECODER

// Some example for algo pipeline
char default_algo_pipeline_desc[] = "yolov1tiny ! opticalflowtrack ! googlenetv2";
//char default_algo_pipeline_desc2[] = "mobilenetssd ! tracklp ! lprnet";
//char default_algo_pipeline_descX[] = "detection ! track name=tk ! tk.vehicle_classification  ! tk.person_face_detection ! face_recognication";

//CvdlFiler properties
enum
{
    PROP_0,
    // Property for algo pipeline, which decide what algo will it run
    PROP_ALGO_PIPELINE_DESC,
    PROP_NUM
};

struct _CvdlFilterPrivate
{
    gboolean negotiated;
    gboolean same_caps_flag;
    GstCaps *inCaps;
    guint width;
    guint height;
};

// Only support NV12 MFXSurface, which need use OpenCL do preprocess
const char cvdl_filter_caps_str[] = \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:MFXSurface", "NV12");

static GstStaticPadTemplate cvdl_sink_factory =
    GST_STATIC_PAD_TEMPLATE (
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (cvdl_filter_caps_str)
    );

static GstStaticPadTemplate cvdl_src_factory =
    GST_STATIC_PAD_TEMPLATE (
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (cvdl_filter_caps_str)
    );


static GstFlowReturn
cvdl_handle_buffer(CvdlFilter *cvdlfilter, GstBuffer* buffer, guint w, guint h)
{
    GstFlowReturn ret = GST_FLOW_OK;

    int cache_buf_size = algo_pipeline_get_all_queue_size(cvdlfilter->algoHandle);
    GST_LOG("cache buffer size = %d\n", cache_buf_size);

#ifdef SYNC_WITH_DECODER
    // wait algo task
    while(cache_buf_size >= 5) {
        g_usleep(10000);// 10ms
        cache_buf_size = algo_pipeline_get_all_queue_size(cvdlfilter->algoHandle);
        GST_LOG("loop - cache buffer size = %d\n", cache_buf_size);
    }
#endif
    // put buffer into a queue
    algo_pipeline_put_buffer(cvdlfilter->algoHandle, buffer, w, h);

    cvdlfilter->frame_num_in++;
    void *data;
    GST_DEBUG("cvdlfilter input: index = %d, buffer = %p, refcount = %d, surface = %d\n",
        cvdlfilter->frame_num_in, buffer, GST_MINI_OBJECT_REFCOUNT (buffer),
        gst_get_mfx_surface(buffer, NULL, &data));

    return ret;;
}


static GstFlowReturn
cvdl_filter_transform_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (parent);
    CvdlFilter *cvdlfilter = CVDL_FILTER (trans);
    CvdlFilterPrivate *priv = cvdlfilter->priv;
    GstClockTime timestamp, duration;

    // Not need ref this buffer, since it will be unrefed in CvdlAlgoBase::queue_buffer(GstBuffer *buffer)
    timestamp = GST_BUFFER_TIMESTAMP (buffer);
    duration = GST_BUFFER_DURATION (buffer);

    if (G_UNLIKELY (!priv->negotiated)) {
        GST_ELEMENT_ERROR (cvdlfilter, CORE, NOT_IMPLEMENTED, (NULL), ("unknown format"));
        return GST_FLOW_NOT_NEGOTIATED;
    }
    GST_LOG_OBJECT (cvdlfilter, "input buffer caps: %" GST_PTR_FORMAT, buffer);

    // Drop this buffer due to algo was not ready now!!!
    if(gst_task_get_state(cvdlfilter->mPushTask) != GST_TASK_STARTED) {
          gst_buffer_unref(buffer);
          g_usleep(1000);//1ms
          return GST_FLOW_OK;
    }

    // Considering perforamce, vision algo task will not be called in main pipeline thread,
    // and it will be called in algo thread:
    //
    // step 1: put input buffer into queue, which will do cvdl processing in another thread
    //         thread 1:  detection thread
    //              --> thread 2: object track thread
    //                    --> thread 3: classification thread
    //                        --> thread 4: output thread
    // step 2: check output queue if there is a output
    //         if there is output, attache it to outbuf
    //         if not, unref outbuf, and set it to be NULL
    cvdl_handle_buffer(cvdlfilter, buffer, priv->width, priv->height);

    if(cvdlfilter->frame_num==0)
        cvdlfilter->startTimePos = g_get_monotonic_time();
    cvdlfilter->frame_num++;
    GST_DEBUG_OBJECT (trans, "src_info: index=%d ",cvdlfilter->frame_num);
    GST_DEBUG_OBJECT (trans, "got buffer with timestamp %" GST_TIME_FORMAT,
                              GST_TIME_ARGS (duration));
    GST_DEBUG ("timestamp %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp));

#ifdef SYNC_WITH_DECODER
    if((duration>0) && (duration<=50000000))
        g_usleep(duration/1000);
    else
        g_usleep(40000);//40ms
#endif
    return GST_FLOW_OK;
}


static GstFlowReturn
cvdl_filter_transform_ip (GstBaseTransform* trans, GstBuffer* inbuf)
{
    return GST_FLOW_OK;
}

static GstCaps *
cvdl_filter_transform_caps (GstBaseTransform* trans,
    GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    GstPad *pad = NULL;
    GstCaps *newcaps = NULL;

    // get the caps of other pad for input caps
    if (GST_PAD_SRC == direction) {
        pad = GST_BASE_TRANSFORM_SINK_PAD (trans);
        // src pad, we set original caps to be sink pad
        newcaps = gst_pad_get_pad_template_caps (pad);
    } else if (GST_PAD_SINK == direction) {
        pad = GST_BASE_TRANSFORM_SRC_PAD (trans);
        // sink pad, we set src pad to be the same with itself
        newcaps = gst_caps_ref (caps);
    }

    if (filter) {
        GstCaps *intersection = gst_caps_intersect_full (filter,
            newcaps, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref (newcaps);
        newcaps = intersection;
    }
    GST_DEBUG_OBJECT (trans, "returning caps: %" GST_PTR_FORMAT, newcaps);

    return newcaps;
}

static void
cvdl_filter_finalize (GObject * object)
{
    CvdlFilter *cvdlfilter = CVDL_FILTER (object);
    CvdlFilterPrivate *priv = cvdlfilter->priv;

    if(priv->inCaps)
        gst_caps_unref(priv->inCaps);
    priv->inCaps=NULL;

    gst_video_info_init (&cvdlfilter->sink_info);
    gst_video_info_init (&cvdlfilter->src_info);

    // stop the data push task
    gst_task_set_state(cvdlfilter->mPushTask, GST_TASK_STOPPED);
    algo_pipeline_flush_buffer(cvdlfilter->algoHandle);
    gst_task_join(cvdlfilter->mPushTask);
    gst_object_unref(cvdlfilter->mPushTask);

    // destroy algo pipeline
    if(cvdlfilter->algoHandle) {
        algo_pipeline_stop(cvdlfilter->algoHandle);
        algo_pipeline_destroy(cvdlfilter->algoHandle);
    }
    cvdlfilter->algoHandle = 0;

    if(cvdlfilter->algo_pipeline_desc)
        g_free(cvdlfilter->algo_pipeline_desc);
    cvdlfilter->algo_pipeline_desc = NULL;
    g_rec_mutex_clear(&cvdlfilter->mMutex);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static char* error_to_string(int code){
    char* error_info = NULL;

    switch(code) {
        case eCvdlFilterErrorCode_IE:
            error_info = g_strdup("IE Error");
            break;
        case eCvdlFilterErrorCode_OCL:
            error_info = g_strdup("OCL Error");
            break;
        case eCvdlFilterErrorCode_EmptyPipe:
        case eCvdlFilterErrorCode_InvalidPipe:
            error_info = g_strdup("pipeline Error");
            break;
        default:
            error_info = g_strdup("Unknown");
            break;
    }
    return error_info;
}

static GstStateChangeReturn
cvdl_filter_change_state (GstElement * element, GstStateChange transition)
{
  CvdlFilter *cvdlfilter = CVDL_FILTER (element);
  CvdlFilterPrivate *priv = cvdlfilter->priv;
  AlgoPipelineConfig *config = NULL;
  int count = 0;
  gint32 duration = 0;
  GstStateChangeReturn result;
  int ret = 0;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        // create the process task(thread) and start it
        if(cvdlfilter->algo_pipeline_desc==NULL)
            config = algo_pipeline_config_create(default_algo_pipeline_desc, &count, element);
        else
            config = algo_pipeline_config_create(cvdlfilter->algo_pipeline_desc, &count, element);

        if(config) {
            cvdlfilter->algoHandle = algo_pipeline_create(config, count, element);
            algo_pipeline_start(cvdlfilter->algoHandle);
            if(config)
                algo_pipeline_config_destroy(config);

            if(priv->inCaps)
                ret = algo_pipeline_set_caps_all(cvdlfilter->algoHandle, priv->inCaps);

            if(ret!=eCvdlFilterErrorCode_None) {
                char* error_info = error_to_string(ret);
                pipeline_report_error_info(element,error_info);
                g_free(error_info);
            }
         } else {
            char *error_info = "Failed to create algo config!";
            pipeline_report_error_info(element,error_info);
            g_print("%s\n", error_info);
         }

         // start push buffer thread to push data to next element
         // It will be done in a task with  push_buffer_func()
        if(gst_task_get_state(cvdlfilter->mPushTask) != GST_TASK_STARTED)
            gst_task_start(cvdlfilter->mPushTask);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        if(gst_task_get_state(cvdlfilter->mWDTask) != GST_TASK_STARTED) {
            cvdlfilter->mQuited = false;
            gst_task_start(cvdlfilter->mWDTask);
        }
        break;
    default:
            break;
  }

  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  switch (transition) {
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        cvdlfilter->mQuited = true;
        gst_task_set_state(cvdlfilter->mWDTask, GST_TASK_STOPPED);
        gst_task_join(cvdlfilter->mWDTask);

        // When change state Playing-->Paused, stop algopipeline,
        //  so that we can Set property for algopipeline.
        // The algo pipeline will have cached buffer to be processed.
        // But it will cause gst-launch failed to change stat to be PLAYING
        // due not buffer was sent to next filter
        //
        //  If we put it to ready or null state, we will encounter below error:
        //  ERROR: GStreamer encountered a general stream error.
        //     [Debug details: qtdemux.c(5520): gst_qtdemux_loop ():
        //      /GstPipeline:pipeline2/GstQTDemux:qtdemux2:
        //     streaming stopped, reason not-linked]
        //
         break;
     case GST_STATE_CHANGE_PAUSED_TO_READY:
         // stop the data push task
         gst_task_set_state(cvdlfilter->mPushTask, GST_TASK_STOPPED);
         algo_pipeline_flush_buffer(cvdlfilter->algoHandle);
         gst_task_join(cvdlfilter->mPushTask);
         if(cvdlfilter->algoHandle) {
            algo_pipeline_stop(cvdlfilter->algoHandle);
            algo_pipeline_destroy(cvdlfilter->algoHandle);
         }
         cvdlfilter->algoHandle = 0;
         break;
    case GST_STATE_CHANGE_READY_TO_NULL:
         cvdlfilter->stopTimePos = g_get_monotonic_time();
         duration = (cvdlfilter->stopTimePos - cvdlfilter->startTimePos)/1000; //ms
         g_print("cvdlfilter processed %d frames in %d seconds, fps = %.2f\n", cvdlfilter->frame_num,
         duration/1000, 1000.0*cvdlfilter->frame_num/duration);
         break;
    default:
         break;
  }

  return result;
}

static void
cvdl_filter_set_property (GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    CvdlFilter *cvdlfilter = CVDL_FILTER (object);

    switch (prop_id) {
        case PROP_ALGO_PIPELINE_DESC:
            cvdlfilter->algo_pipeline_desc = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
    return;
}

static void
cvdl_filter_get_property (GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    CvdlFilter *cvdlfilter = CVDL_FILTER (object);

    switch (prop_id) {
        case PROP_ALGO_PIPELINE_DESC:
            if(cvdlfilter->algo_pipeline_desc)
                g_value_set_string (value, cvdlfilter->algo_pipeline_desc);
            else
                g_value_set_string (value, default_algo_pipeline_desc);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean
cvdl_filter_propose_allocation (GstBaseTransform* trans, GstQuery* decide_query, GstQuery* query)
{
   GstCaps *caps = NULL;
   gboolean need_pool = FALSE;

   gst_query_parse_allocation (query, &caps, &need_pool);
   if (!caps) {
       GST_DEBUG_OBJECT (trans, "no caps specified");
       return FALSE;
   }

   GstVideoInfo info;
   if (!gst_video_info_from_caps (&info, caps)) {
       GST_DEBUG_OBJECT (trans, "invalid caps specified");
       return FALSE;
   }

   if (need_pool) {
       GST_DEBUG_OBJECT (trans, "need_pool should be false");
       return FALSE;
   }

   gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);
   gst_query_add_allocation_meta (query, GST_VIDEO_CROP_META_API_TYPE, NULL);

   return TRUE;
}

static gboolean
cvdl_filter_decide_allocation (GstBaseTransform* trans, GstQuery* query)
{
    GstBufferPool *pool = NULL;
    //gboolean update_pool = FALSE;
    guint min = 0, max = 0, size = 0;

    if (gst_query_get_n_allocation_pools (query) > 0) {
        gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
        //update_pool = TRUE;
        GST_DEBUG_OBJECT (trans, "decide_allocation - update_pool should be false.");
        return FALSE;
    }

    GstCaps *caps;
    gboolean need_pool = FALSE;
    gst_query_parse_allocation (query, &caps, &need_pool);
    if (!caps) {
        GST_DEBUG_OBJECT (trans, "no caps specified");
        return FALSE;
    }

    if (need_pool) {
        GST_DEBUG_OBJECT (trans, "decide_allocation - need_pool should be false");
        return FALSE;
    }

    // we also support various metadata
    gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);
    gst_query_add_allocation_meta (query, GST_VIDEO_CROP_META_API_TYPE, NULL);

    return GST_BASE_TRANSFORM_CLASS (parent_class)->decide_allocation (trans, query);
}

static gboolean
cvdl_filter_caps_negotiation (const CvdlFilter* cvdlfilter)
{
    const GstVideoInfo *sink_info = &cvdlfilter->sink_info;
    const GstVideoInfo *src_info = &cvdlfilter->src_info;

    // Only NV12 supported at present
    if (GST_VIDEO_INFO_FORMAT (sink_info) != GST_VIDEO_INFO_FORMAT (src_info) ||
        GST_VIDEO_INFO_FORMAT (sink_info) != GST_VIDEO_FORMAT_NV12) {
        GST_ERROR_OBJECT (cvdlfilter, "CvdlFilter only support NV12 frame at present");
        return FALSE;
    }

    return TRUE;
}

static gboolean
gst_compare_caps (GstCaps* incaps, GstCaps* outcaps)
{
    if (!incaps || !outcaps)
        return FALSE;

    GstStructure *s1 = gst_caps_get_structure (incaps, 0);
    GstStructure *s2 = gst_caps_get_structure (outcaps, 0);
    GstCapsFeatures *f1 = gst_caps_get_features (incaps, 0);
    if (!f1) {
        f1 = GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY;
    }

    GstCapsFeatures *f2 = gst_caps_get_features (outcaps, 0);
    if (!f2) {
        f2 = GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY;
    }

    if (!gst_caps_features_is_equal (f1, f2)) {
        return FALSE;
    }

    if (g_ascii_strcasecmp (gst_structure_get_name (s1), gst_structure_get_name (s2))) {
        return FALSE;
    }

    if (gst_value_compare (gst_structure_get_value (s1, "format"),
                           gst_structure_get_value (s2, "format")) ||
        gst_value_compare (gst_structure_get_value (s1, "width"),
                           gst_structure_get_value (s2, "width")) ||
        gst_value_compare (gst_structure_get_value (s1, "height"),
                           gst_structure_get_value (s2, "height"))) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
cvdl_filter_set_caps (GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps)
{
    CvdlFilter *cvdlfilter = CVDL_FILTER (trans);
    CvdlFilterPrivate *priv = cvdlfilter->priv;
    GstVideoInfo info;

    if (!gst_video_info_from_caps (&cvdlfilter->sink_info, incaps) ||
        !gst_video_info_from_caps (&cvdlfilter->src_info, outcaps)) {
        GST_ERROR_OBJECT (cvdlfilter, "invalid caps");
        return FALSE;
    }

    priv->negotiated = cvdl_filter_caps_negotiation (cvdlfilter);
    if (!priv->negotiated) {
        GST_ERROR_OBJECT (cvdlfilter, "cvdl_filter_caps_negotiation failed");
        return FALSE;
    }

    priv->same_caps_flag = gst_compare_caps (incaps, outcaps);
    gst_base_transform_set_passthrough (trans, priv->same_caps_flag);

    gst_video_info_from_caps (&info, incaps);
    priv->width = info.width;
    priv->height = info.height;

    priv->inCaps = gst_caps_ref(incaps);
    algo_pipeline_set_caps_all(cvdlfilter->algoHandle, incaps);

    return TRUE;
}

static GstCaps*
cvdl_filter_fixate_caps (GstBaseTransform* trans,
    GstPadDirection direction, GstCaps* caps, GstCaps* othercaps)
{
    return GST_BASE_TRANSFORM_CLASS (parent_class) ->fixate_caps (trans, direction, caps, othercaps);
}

static void
cvdl_filter_class_init (CvdlFilterClass * klass)
{
    GObjectClass *obj_class = (GObjectClass*) klass;
    GstElementClass *elem_class = (GstElementClass*) klass;
    GstBaseTransformClass *trans_class = (GstBaseTransformClass*) klass;

    GST_DEBUG_CATEGORY_INIT (cvdl_filter_debug, "cvdlfilter", 0,
        "Process video image with CV/DL algorithm.");
    GST_DEBUG ("cvdl_filter_class_init");

    g_type_class_add_private (klass, sizeof (CvdlFilterPrivate));
    obj_class->finalize     = cvdl_filter_finalize;
    obj_class->set_property = cvdl_filter_set_property;
    obj_class->get_property = cvdl_filter_get_property;

    elem_class->change_state = cvdl_filter_change_state;
    gst_element_class_set_details_simple (elem_class,
        "CvdlFilter", "Filter/Video/CVDL",
        "Video CV/DL algorithm processing based on OpenVINO",
        "River,Li <river.li@intel.com>");

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ALGO_PIPELINE_DESC,
         g_param_spec_string ("algopipeline", "AlgoPipelineDesc",
             "CV/DL algo pipeline string, support:  yolov1tiny, opticalflowtrack, googlenetv2, mobilenetssd, tracklp, lprnet, reid, yolov2tiny",
             "yolov1tiny ! opticalflowtrack ! googlenetv2",
             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_add_pad_template (elem_class, gst_static_pad_template_get (&cvdl_src_factory));
    gst_element_class_add_pad_template (elem_class, gst_static_pad_template_get (&cvdl_sink_factory));

    trans_class->propose_allocation = cvdl_filter_propose_allocation;
    trans_class->decide_allocation  = cvdl_filter_decide_allocation;
    trans_class->transform_caps     = cvdl_filter_transform_caps;
    trans_class->fixate_caps        = cvdl_filter_fixate_caps;
    trans_class->set_caps           = cvdl_filter_set_caps;

    //If no transform function, always_in_place is TRUE
    //trans_class->transform          = NULL;
    trans_class->transform_ip       = cvdl_filter_transform_ip;
}

// Main function to push processed buffer into next filter
static void push_buffer_func(gpointer userData)
{
    CvdlFilter* cvdl_filter = (CvdlFilter* )userData;
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (userData);
    GstBuffer *outbuf = NULL;
    void *data = NULL;

    // get data from output queue, and attach it into outbuf
    // if no data in output queue, return NULL;
    algo_pipeline_get_buffer(cvdl_filter->algoHandle, &outbuf);

    cvdl_filter->frame_num_out++;
    if(outbuf) {
        GST_DEBUG("cvdlfilter out: index = %d, buffer = %p, refcount = %d, surface = %d\n",
            cvdl_filter->frame_num_out, outbuf, GST_MINI_OBJECT_REFCOUNT(outbuf),
            gst_get_mfx_surface(outbuf, NULL, &data));
    }

    //push the buffer only when can get an output data 
    if(outbuf) {
        gst_pad_push(trans->srcpad, outbuf);
        // gst_pad_push will unref this buffer, we need not do it again.
    }
}


// watch dog thread
static void watch_dog_func(gpointer userData)
{
    CvdlFilter* cvdl_filter = (CvdlFilter* )userData;
    int num = 200;
    int current_frame_num = cvdl_filter->frame_num;

    while(num-->0 && !cvdl_filter->mQuited) {
        if(cvdl_filter->frame_num > current_frame_num)
            break;
        g_usleep(500000); //500ms
    }

    if(num<=0) {
        cvdl_filter->mQuited = true;
        char *error_info = "Not get data for too long!\n";
        pipeline_report_error_info((GstElement *)cvdl_filter,error_info);
    }
}

static void
cvdl_filter_init (CvdlFilter* cvdl_filter)
{
    CvdlFilterPrivate *priv = cvdl_filter->priv = CVDL_FILTER_GET_PRIVATE (cvdl_filter);
    GstBaseTransform *trans = (GstBaseTransform *)cvdl_filter;

    gst_pad_set_chain_function (trans->sinkpad, GST_DEBUG_FUNCPTR (cvdl_filter_transform_chain));

    priv->negotiated = FALSE;
    priv->same_caps_flag = FALSE;
    priv->inCaps = NULL;

    gst_video_info_init (&cvdl_filter->sink_info);
    gst_video_info_init (&cvdl_filter->src_info);

    cvdl_filter->frame_num = 0;
    cvdl_filter->startTimePos = g_get_monotonic_time();
    cvdl_filter->mQuited = false;

    // create task for push data to next filter/element
    g_rec_mutex_init (&cvdl_filter->mMutex);
    cvdl_filter->mPushTask = gst_task_new (push_buffer_func, (gpointer)cvdl_filter, NULL);
    gst_task_set_lock (cvdl_filter->mPushTask, &cvdl_filter->mMutex);
    gst_task_set_enter_callback (cvdl_filter->mPushTask, NULL, NULL, NULL);
    gst_task_set_leave_callback (cvdl_filter->mPushTask, NULL, NULL, NULL);

    // create Watch dog task
    g_rec_mutex_init (&cvdl_filter->mWDMutex);
    cvdl_filter->mWDTask = gst_task_new (watch_dog_func, (gpointer)cvdl_filter, NULL);
    gst_task_set_lock (cvdl_filter->mWDTask, &cvdl_filter->mWDMutex);
    gst_task_set_enter_callback (cvdl_filter->mWDTask, NULL, NULL, NULL);
    gst_task_set_leave_callback (cvdl_filter->mWDTask, NULL, NULL, NULL);
}
