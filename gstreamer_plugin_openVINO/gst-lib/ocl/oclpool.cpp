/*
 * Copyright (c) 2017 Intel Corporation
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

#include <unistd.h>
#include <string.h>
#include <va/va_drmcommon.h>
#include <gst/allocators/gstdmabuf.h>
#include "va/va.h"

#include "oclpool.h"
#include "oclmemory.h"
#include "interface/vpphost.h"
#include "common/macros.h"

#include <CL/cl.h>

using namespace cv;
struct _OclPoolPrivate
{
    GstCaps *caps;
    GstVideoInfo info;
    GstVideoAlignment align;

    gboolean add_videometa;
    gboolean need_alignment;
    GstAllocator *allocator;
    GstAllocationParams params;
};

static const guint BPP = 32;
static const guint BPP_FOR_BGR = 1;  // bytes per pxial for NV12 frame
static const guint UNTILED_NV12_ALIGN_BYTES = 128;   // bytes of width alignment for untiled NV12 frame

#define GST_CAT_DEFAULT ocl_pool_debug
GST_DEBUG_CATEGORY_STATIC (ocl_pool_debug);

#define ocl_pool_parent_class parent_class
G_DEFINE_TYPE (OclPool, ocl_pool, GST_TYPE_BUFFER_POOL);


static gboolean
ocl_pool_set_config (GstBufferPool* pool, GstStructure* config)
{
    GstCaps *caps;
    guint size, min, max;

    if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min, &max)) {
        GST_WARNING_OBJECT (pool, "invalid config");
        return FALSE;
    }

    if (!caps) {
        GST_WARNING_OBJECT (pool, "no caps in config");
        return FALSE;
    }

    GstVideoInfo info;
    if (!gst_video_info_from_caps (&info, caps)) {
        GST_WARNING_OBJECT (pool, "failed getting geometry from caps %" GST_PTR_FORMAT, caps);
        return FALSE;
    }

    //FIXME: support BGR3 and GRAY8 only
    if( (info.finfo->format != GST_VIDEO_FORMAT_GRAY8) &&
        (info.finfo->format != GST_VIDEO_FORMAT_BGR) ) {
        GST_WARNING_OBJECT (pool, "Got invalid format when config pool!");
        return FALSE;
    }

    if (size < info.size) {
        GST_WARNING_OBJECT (pool, "Provided size is to small for the caps: %u", size);
        return FALSE;
    }
    GST_LOG_OBJECT (pool, "%dx%d, caps %" GST_PTR_FORMAT, info.width, info.height, caps);

    OclPoolPrivate *priv = OCL_POOL_CAST (pool)->priv;
    if (priv->caps)
        gst_caps_unref (priv->caps);

    priv->caps = gst_caps_ref (caps);
    priv->add_videometa = TRUE;   //TODO

    // Only support BRG/Gray8 format
    info.offset[1] = info.offset[2] = 0;
    if(info.finfo->format == GST_VIDEO_FORMAT_BGR)
        info.stride[0] = info.width * 3;
    else if(info.finfo->format == GST_VIDEO_FORMAT_GRAY8)
        info.stride[0] = info.width;
    info.size = MAX (size, info.size);
    priv->info = info;

    GST_DEBUG_OBJECT (pool, "%dx%d, sz: %zu, offset: %zu, %zu, stride: %d, %d",
        info.width, info.height, info.size, info.offset[0], info.offset[1],
        info.stride[0], info.stride[1]);

    gst_buffer_pool_config_set_params (config, caps, info.size, min, max);

    return GST_BUFFER_POOL_CLASS (parent_class)->set_config (pool, config);
}


#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)


static GstMemory*
ocl_memory_alloc (OclPool* oclpool)
{
    OclPoolPrivate *priv = oclpool->priv;
    OclMemory *ocl_mem = g_new0(OclMemory, 1);

    OCL_MEMORY_FOURCC (ocl_mem) = OCL_FOURCC_BGR3;
    OCL_MEMORY_WIDTH (ocl_mem) = GST_VIDEO_INFO_WIDTH (&priv->info);
    OCL_MEMORY_HEIGHT (ocl_mem) = GST_VIDEO_INFO_HEIGHT (&priv->info);
    OCL_MEMORY_SIZE (ocl_mem) = GST_VIDEO_INFO_SIZE (&priv->info);

    switch(priv->info.finfo->format) {
        case GST_VIDEO_FORMAT_GRAY8:
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC1);
            break;
        case GST_VIDEO_FORMAT_BGR:
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC3);
            break;
        case GST_VIDEO_FORMAT_BGRx:
            // For blender module
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC4);
            break;
        default:
            g_print("Not support format = %d\n", priv->info.finfo->format);
            while(1);
            break;
    }
    g_return_val_if_fail(ocl_mem->frame.offset == 0, NULL);
    g_return_val_if_fail(ocl_mem->frame.isContinuous(), NULL);

    // CRC is ACCESS_WRITE, but blender is ACCESS_READ
    OCL_MEMORY_MEM (ocl_mem) = (cl_mem)ocl_mem->frame.handle(ACCESS_RW);

    GstMemory *memory = GST_MEMORY_CAST (ocl_mem);
    gst_memory_init (memory, GST_MEMORY_FLAG_NO_SHARE, priv->allocator, NULL,
          GST_VIDEO_INFO_SIZE (&priv->info), 0, 0, GST_VIDEO_INFO_SIZE (&priv->info)); //TODO

    return memory;
}

static GstFlowReturn
ocl_pool_alloc (GstBufferPool* pool, GstBuffer** buffer,
    GstBufferPoolAcquireParams* params)
{
    GstBuffer *ocl_buf;
    OclMemory *ocl_mem;

    OclPool *oclpool = OCL_POOL_CAST (pool);
    OclPoolPrivate *priv = oclpool->priv;

    ocl_buf = gst_buffer_new ();
    ocl_mem = (OclMemory *) oclpool->memory_alloc (oclpool);
    if (!ocl_mem) {
        gst_buffer_unref (ocl_buf);
        GST_ERROR_OBJECT (pool, "failed to alloc ocl memory");
        return GST_FLOW_ERROR;
    }

    gst_mini_object_set_qdata (GST_MINI_OBJECT_CAST (ocl_mem),
        OCL_MEMORY_QUARK, GST_MEMORY_CAST (ocl_mem),
        (GDestroyNotify) gst_memory_unref);

    gst_buffer_append_memory (ocl_buf, (GstMemory *)ocl_mem);

    GstVideoInfo *info = &priv->info;

    GST_DEBUG_OBJECT (pool,
        " %dx%d, sz: %zu, offset: %zu, %zu, stride: %d, %d",
        info->width, info->height, info->size,
        info->offset[0], info->offset[1], info->stride[0], info->stride[1]);

    if (priv->add_videometa) {
        GST_DEBUG_OBJECT (pool, "adding GstVideoMeta");
        /* these are just the defaults for now */
        gst_buffer_add_video_meta_full (ocl_buf, GST_VIDEO_FRAME_FLAG_NONE,
        GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
        GST_VIDEO_INFO_HEIGHT (info), GST_VIDEO_INFO_N_PLANES (info),
        info->offset, info->stride);
    }
    *buffer = ocl_buf;

    return GST_FLOW_OK;
}

static void
ocl_pool_finalize (GObject* object)
{
    OclPool *pool = OCL_POOL_CAST (object);
    OclPoolPrivate *priv = pool->priv;

    GST_LOG_OBJECT (pool, "finalize ocl pool %p", pool);

    if (priv->caps)
        gst_caps_unref (priv->caps);

    if (priv->allocator)
        gst_object_unref (priv->allocator);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ocl_pool_class_init (OclPoolClass* klass)
{
    GObjectClass *oclass = (GObjectClass *) klass;
    GstBufferPoolClass *pclass = (GstBufferPoolClass *) klass;

    g_type_class_add_private (klass, sizeof (OclPoolPrivate));

    oclass->finalize = ocl_pool_finalize;

    //gstbufferpool_class->get_options = ocl_pool_get_options;
    pclass->set_config = ocl_pool_set_config;
    pclass->alloc_buffer = ocl_pool_alloc;

    GST_DEBUG_CATEGORY_INIT (ocl_pool_debug, "ocl_pool", 0, "ocl_pool object");
}

static void
ocl_pool_init (OclPool* pool)
{
    pool->priv = OCL_POOL_GET_PRIVATE (pool);
}

static GstBufferPool*
ocl_pool_new (OclMemoryType mem_type)
{
    OclPool *pool;
    pool = (OclPool*) g_object_new (GST_TYPE_OCL_POOL, NULL);

    pool->priv->allocator = ocl_allocator_new (mem_type);

    return GST_BUFFER_POOL_CAST (pool);
}

GstBufferPool*
ocl_pool_create (GstCaps* caps, gsize size, gint min, gint max)
{
    GstBufferPool *pool = ocl_pool_new (OCL_MEMORY_OCV);
    //OCL_POOL_CAST(pool)->priv = OCL_POOL_GET_PRIVATE (pool);

    //g_return_val_if_fail(mem_type == OCL_MEMORY_OCV, NULL);
    OCL_POOL_CAST(pool)->memory_alloc = ocl_memory_alloc;

    GstStructure *config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

    if (!gst_buffer_pool_set_config (pool, config)) {
        gst_object_unref (pool);
        pool = NULL;
    }

    return pool;
}
