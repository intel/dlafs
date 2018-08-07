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

#include "cvdlfilter.h"


GST_DEBUG_CATEGORY_STATIC (cvdl_filter_debug);
#define GST_CAT_DEFAULT cvdl_filter_debug


#define cvdl_filter_parent_class parent_class
G_DEFINE_TYPE (CvdlFilter, cvdl_filter, GST_TYPE_BASE_TRANSFORM);

#define CVDL_FILTER_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CVDL_FILTER_TYPE, CvdlFilterPrivate))

/* GstVideoFlip properties */
enum
{
    PROP_0,
    PROP_METHOD
};

struct _CvdlFilterPrivate
{
    //VideoFrame *src_frame;
    //VideoFrame *dst_frame;
    gboolean negotiated;
    gboolean same_caps_flag;

//    SharedPtr<OclContext> ctx;
//    SharedPtr<VppInterface> vpp_crc;
//    gboolean cl_inited;
};

const char cvdl_filter_caps_str[] = \
    GST_VIDEO_CAPS_MAKE ("NV12") "; " \
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

/*
static void cvdl_ocl_context_init(CvdlFilter *cvdlfilter, VADisplay display)
{
    CvdlFilterPrivate *priv = cvdlfilter->priv;

    if(priv->cl_inited)
        return;

    priv->ctx = OclContext::create(display);
    if (!SHARED_PTR_IS_VALID (priv->ctx)) {
        GST_DEBUG ("oclcrc: failed to create ocl ctx");
        return;
    }

    priv->vpp_crc.reset (NEW_VPP_SHARED_PTR (OCL_VPP_CRC));
    if (!SHARED_PTR_IS_VALID (priv->vpp_crc) ||
        (OCL_SUCCESS != priv->vpp_crc->setOclContext (priv->ctx))) {
        GST_DEBUG ("oclcrc: failed to init vpp_crc");
        priv->vpp_crc.reset ();
    }

    priv->cl_inited = true;
}
*/

static GstFlowReturn
cvdl_handle_buffer(CvdlFilter *cvdlfilter, GstBuffer* buffer)
{
    GstFlowReturn ret = GST_FLOW_OK;

    // put buffer into a queue
    //cvdlfilter->first_algo->queue_buffer(buffer);
    algo_pipeline_put_buffer(cvdlfilter->algoHandle, buffer);

    return ret;;
}

GstBuffer* cvdl_try_to_get_output(CvdlFilter *cvdlfilter)
{
    GstBuffer *buf = NULL;
    // get data from output queue, and attach it into outbuf
    // if no data in output queue, return NULL;

    //buf = cvdlfilter->last_algo->dequeue_buffer();
    algo_pipeline_get_buffer(cvdlfilter->algoHandle, &buf);

    return buf;
}

static GstFlowReturn
cvdl_filter_transform_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (parent);
    //GstBaseTransformClass *klass = GST_BASE_TRANSFORM_GET_CLASS (trans);
    CvdlFilter *cvdlfilter = CVDL_FILTER (trans);
    CvdlFilterPrivate *priv = cvdlfilter->priv;
    //VASurfaceID surface = VA_INVALID_SURFACE;
    //GstFlowReturn ret;
    //GstClockTime position = GST_CLOCK_TIME_NONE;
    GstClockTime timestamp, duration;
    //GstBuffer *outbuf = NULL;
    
    timestamp = GST_BUFFER_TIMESTAMP (buffer);
    duration = GST_BUFFER_DURATION (buffer);

    if (G_UNLIKELY (!priv->negotiated)) {
        GST_ELEMENT_ERROR (cvdlfilter, CORE, NOT_IMPLEMENTED, (NULL), ("unknown format"));
        return GST_FLOW_NOT_NEGOTIATED;
    }
    GST_LOG_OBJECT (cvdlfilter, "input buffer caps: %" GST_PTR_FORMAT, buffer);

    /* vpp will not called in main pipeline thread, but it should be called in algo thread*/
    //if (!SHARED_PTR_IS_VALID (priv->vpp_crc)) {
    //    GST_ERROR_OBJECT (trans, "invalid crc");
    //    return GST_FLOW_ERROR;
    //}

    //priv->src_frame->surface= 
    //            gst_get_mfx_surface (buffer, cvdlfilter->sink_info, &cvdlfilter->display);

    //if(!priv->cl_inited) {
    //    cvdl_ocl_context_init(cvdlfilter, display);
    //}

    //priv->src_frame->fourcc =
    //            video_format_to_va_fourcc (GST_VIDEO_INFO_FORMAT (&cvdlfilter->src_info));
    //if (priv->src_frame->fourcc != OCL_FOURCC_NV12) {
    //    GST_ERROR ("only support NV12 video frame");
    //    return GST_FLOW_ERROR;
    //}

    // step 1: put input buffer into queue, which will do cvdl processing in another thread
    //         thread 1:  detection thread
    //              --> thread 2: object track thread
    //                    --> thread 3: classification thread
    //                        --> thread 4: output thread
    // step 2: check output queue if there is a output
    //         if there is output, attache it to outbuf
    //         if not, unref outbuf, and set it to be NULL
    cvdl_handle_buffer(cvdlfilter, buffer);

    // It will be done in a task with  push_buffer_func()
    if(gst_task_get_state(cvdlfilter->mPushTask) != GST_TASK_STARTED)
        gst_task_start(cvdlfilter->mPushTask);
    //outbuf = cvdl_try_to_get_output(cvdlfilter);
    //push the buffer only when can get a output data 
    //if(outbuf)
    //    gst_pad_push(trans->srcpad, outbuf);

    cvdlfilter->frame_num++;
    GST_DEBUG_OBJECT (trans, "src_info: index=%d ",cvdlfilter->frame_num);
    GST_DEBUG_OBJECT (trans, "got buffer with timestamp %" GST_TIME_FORMAT,
                                              GST_TIME_ARGS (duration));
    GST_DEBUG ("timestamp %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp));

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

    if (GST_PAD_SRC == direction) {
        pad = GST_BASE_TRANSFORM_SINK_PAD (trans);
    } else if (GST_PAD_SINK == direction) {
        pad = GST_BASE_TRANSFORM_SRC_PAD (trans);
    }

    GstCaps *newcaps = gst_pad_get_pad_template_caps (pad);

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

    //gst_object_unref (cvdlfilter->sink_pool);
    gst_video_info_init (&cvdlfilter->sink_info);
    gst_video_info_init (&cvdlfilter->src_info);

    // stop the data push task
    gst_task_set_state(cvdlfilter->mPushTask, GST_TASK_STOPPED);
    gst_task_join(cvdlfilter->mPushTask);

#if 0
    for(int i=0; i<CVDL_ALGO_NUM; i++)
        if(cvdlfilter->algo[i])
            delete cvdlfilter->algo[i];
#else
    algo_pipeline_destroy(cvdlfilter->algoHandle);
#endif
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cvdl_filter_set_property (GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    return;
    //gst_base_transform_reconfigure_src (GST_BASE_TRANSFORM (plugin));
}

static void
cvdl_filter_get_property (GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static gboolean
cvdl_filter_propose_allocation (GstBaseTransform* trans, GstQuery* decide_query, GstQuery* query)
{
   GstCaps *caps = NULL;
   gboolean need_pool = FALSE;

   //CvdlFilter *cvdlfilter = CVDL_FILTER (trans);

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
    gboolean update_pool = FALSE;
    guint min = 0, max = 0, size = 0;

    if (gst_query_get_n_allocation_pools (query) > 0) {
        gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
        update_pool = TRUE;
        GST_DEBUG_OBJECT (trans, "decide_allocation - update_pool should be false, but it is %d",update_pool);
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

    /* we also support various metadata */
    gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);
    gst_query_add_allocation_meta (query, GST_VIDEO_CROP_META_API_TYPE, NULL);

    return GST_BASE_TRANSFORM_CLASS (parent_class)->decide_allocation (trans, query);
}

static gboolean
cvdl_filter_caps_negotiation (const CvdlFilter* cvdlfilter)
{
    const GstVideoInfo *sink_info = &cvdlfilter->sink_info;
    const GstVideoInfo *src_info = &cvdlfilter->src_info;

    // TODO: only NV12 supported at present
    if (GST_VIDEO_INFO_FORMAT (sink_info) != GST_VIDEO_INFO_FORMAT (src_info) ||
        GST_VIDEO_INFO_FORMAT (sink_info) != GST_VIDEO_FORMAT_NV12) {
        GST_ERROR_OBJECT (cvdlfilter, "CvdlFilter only support on NV12 frame at present");
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
        //gst_value_compare (gst_structure_get_value (s1, "tiled"),
        //                   gst_structure_get_value (s2, "tiled")) ||
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

#if 0
    // this 3 algo accept the orignal video buffer
    cvdlfilter->algo[ALGO_DETECTION]->set_data_caps(incaps);
    cvdlfilter->algo[ALGO_TRACKING]->set_data_caps(incaps);
    cvdlfilter->algo[ALGO_CLASSIFICATION]->set_data_caps(incaps);
#else
    algo_pipeline_set_caps_all(cvdlfilter->algoHandle, incaps);
#endif
    return TRUE;
}

static GstCaps*
cvdl_filter_fixate_caps (GstBaseTransform* trans,
    GstPadDirection direction, GstCaps* caps, GstCaps* othercaps)
{
    //ocl_fixate_caps (othercaps);

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

    gst_element_class_set_details_simple (elem_class,
        "CvdlFilter", "Filter/Video/CVDL",
        "Video CV/DL algorithm processing based on OpenVINO",
        "River,Li <river.li@intel.com>");

    gst_element_class_add_pad_template (elem_class, gst_static_pad_template_get (&cvdl_src_factory));
    gst_element_class_add_pad_template (elem_class, gst_static_pad_template_get (&cvdl_sink_factory));

    trans_class->propose_allocation = cvdl_filter_propose_allocation;
    trans_class->decide_allocation  = cvdl_filter_decide_allocation;
    trans_class->transform_caps     = cvdl_filter_transform_caps;
    trans_class->fixate_caps        = cvdl_filter_fixate_caps;
    trans_class->set_caps           = cvdl_filter_set_caps;

    /* If no transform function, always_in_place is TRUE */
    //trans_class->transform          = NULL;
    trans_class->transform_ip       = cvdl_filter_transform_ip;
}

// Main function to push processed buffer into next filter
static void push_buffer_func(gpointer userData)
{
    CvdlFilter* cvdl_filter = (CvdlFilter* )userData;
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (userData);
    GstBuffer *outbuf;

    outbuf = cvdl_try_to_get_output(cvdl_filter);

    //push the buffer only when can get an output data 
    if(outbuf)
        gst_pad_push(trans->srcpad, outbuf);
}


static void
cvdl_filter_init (CvdlFilter* cvdl_filter)
{
    CvdlFilterPrivate *priv = cvdl_filter->priv = CVDL_FILTER_GET_PRIVATE (cvdl_filter);
    GstBaseTransform *trans = (GstBaseTransform *)cvdl_filter;

    gst_pad_set_chain_function (trans->sinkpad, GST_DEBUG_FUNCPTR (cvdl_filter_transform_chain));

    priv->negotiated = FALSE;
    priv->same_caps_flag = FALSE;

    //priv->src_frame.reset (g_new0 (VideoFrame, 1), g_free);
    //priv->dst_frame.reset (g_new0 (VideoFrame, 1), g_free);

    //cvdl_filter->detection_pool = NULL;
    //cvdl_filter->tracking_pool = NULL;
    //cvdl_filter->classification_pool = NULL:
    gst_video_info_init (&cvdl_filter->sink_info);
    gst_video_info_init (&cvdl_filter->src_info);

    cvdl_filter->frame_num = 0;

    //cvdl_filter->display = VA_INVALID_DISPLAY;

    // create task for push data to next filter/element
    g_rec_mutex_init (&cvdl_filter->mMutex);
    cvdl_filter->mPushTask = gst_task_new (push_buffer_func, (gpointer)cvdl_filter, NULL);
    gst_task_set_lock (cvdl_filter->mPushTask, &cvdl_filter->mMutex);
    gst_task_set_enter_callback (cvdl_filter->mPushTask, NULL, NULL, NULL);
    gst_task_set_leave_callback (cvdl_filter->mPushTask, NULL, NULL, NULL);

#if 0
    // create algo and link them
    cvdl_filter->algo[ALGO_DETECTION] = new DetectionAlgo;
    cvdl_filter->algo[ALGO_TRACKING] = new TrackAlgo;
    cvdl_filter->algo[ALGO_CLASSIFICATION] = new ClassificationAlgo;

    cvdl_filter->algo[ALGO_DETECTION]->algo_connect(cvdl_filter->algo[ALGO_TRACKING]);
    cvdl_filter->algo[ALGO_TRACKING]->algo_connect(cvdl_filter->algo[ALGO_CLASSIFICATION]);
    cvdl_filter->algo[ALGO_CLASSIFICATION]->algo_connect(NULL);

    cvdl_filter->first_algo = cvdl_filter->algo[ALGO_DETECTION];
    cvdl_filter->last_algo  = cvdl_filter->algo[ALGO_CLASSIFICATION];

    //start algo thread
    for(int i=0; i<CVDL_ALGO_NUM; i++)
        cvdl_filter->algo[ALGO_DETECTION].start_algo_thread();
#else
    cvdl_filter->algoHandle = algo_pipeline_create_default();
    algo_pipeline_start(cvdl_filter->algoHandle);
#endif
}
