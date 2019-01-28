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

#include <unistd.h>
#include <string.h>
#include <gst/allocators/gstdmabuf.h>

#include "oclpool.h"
#include "oclmemory.h"
#include "interface/vpphost.h"

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

    //FIXME: support BGR/BGRA and GRAY8 only
    if( (info.finfo->format != GST_VIDEO_FORMAT_GRAY8) &&
        (info.finfo->format != GST_VIDEO_FORMAT_BGRA) &&
        (info.finfo->format != GST_VIDEO_FORMAT_BGR) ) {
        GST_WARNING_OBJECT (pool, "Got invalid format when config pool!");
        GST_ERROR("%s() - got invalid format when config pool, format=%d\n",__func__, info.finfo->format);
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
    priv->add_videometa = TRUE;

    // Only support BGRA/BRG/Gray8 format
    info.offset[1] = info.offset[2] = 0;
    if(info.finfo->format == GST_VIDEO_FORMAT_BGR)
        info.stride[0] = info.width * 3;
    else if(info.finfo->format == GST_VIDEO_FORMAT_BGRA)
        info.stride[0] = info.width * 4;
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
    //OclMemory *ocl_mem = g_new0(OclMemory, 1);
    OclMemory *ocl_mem = new OclMemory;
    if(ocl_mem==NULL) {
        g_print("Failed to allocate OclMemory!\n");
        return NULL;
    }

    OCL_MEMORY_WIDTH (ocl_mem) = GST_VIDEO_INFO_WIDTH (&priv->info);
    OCL_MEMORY_HEIGHT (ocl_mem) = GST_VIDEO_INFO_HEIGHT (&priv->info);
    OCL_MEMORY_SIZE (ocl_mem) = GST_VIDEO_INFO_SIZE (&priv->info);

    switch(priv->info.finfo->format) {
        case GST_VIDEO_FORMAT_GRAY8:
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC1);
            OCL_MEMORY_FOURCC (ocl_mem) = OCL_FOURCC_GRAY;
            ocl_mem->mem_size = OCL_MEMORY_WIDTH (ocl_mem) * OCL_MEMORY_HEIGHT (ocl_mem);
            break;
        case GST_VIDEO_FORMAT_BGR:
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC3);
            OCL_MEMORY_FOURCC (ocl_mem) = OCL_FOURCC_BGR3;
            ocl_mem->mem_size = OCL_MEMORY_WIDTH (ocl_mem) * OCL_MEMORY_HEIGHT (ocl_mem) * 3;
            break;
        case GST_VIDEO_FORMAT_BGRA:
            // For blender module
            ocl_mem->frame.create(cv::Size(OCL_MEMORY_WIDTH (ocl_mem),OCL_MEMORY_HEIGHT (ocl_mem)), CV_8UC4);
            OCL_MEMORY_FOURCC (ocl_mem) = OCL_FOURCC_BGRA;
            ocl_mem->mem_size = OCL_MEMORY_WIDTH (ocl_mem) * OCL_MEMORY_HEIGHT (ocl_mem) * 4;
            break;
        default:
            GST_ERROR("Not support format = %d\n", priv->info.finfo->format);
            ocl_mem->frame =  cv::UMat();
            ocl_mem->mem_size = 0;
            break;
    }
    //g_return_val_if_fail(ocl_mem->frame.offset == 0, NULL);
    //g_return_val_if_fail(ocl_mem->frame.isContinuous(), NULL);
    if(!(ocl_mem->frame.offset==0) || !(ocl_mem->frame.isContinuous())) {
        ocl_mem->frame.release();
        delete ocl_mem;
        g_print("ocl_mem->frame.create failed!\n");
        return NULL;
    }

    if(priv->info.finfo->format == GST_VIDEO_FORMAT_BGRA) {
         // OclVppBlender::blend_helper () - clCreateImage2D
         //OCL_MEMORY_MEM (ocl_mem) = (cl_mem)ocl_mem->frame.getMat(0).ptr();//System address
         OCL_MEMORY_MEM (ocl_mem) = (cl_mem)ocl_mem->frame.handle(ACCESS_RW);//CL address
    }
    else
    {
        // CRC is ACCESS_WRITE, but blender is ACCESS_READ or ACCESS_WRITE
        OCL_MEMORY_MEM (ocl_mem) = (cl_mem)ocl_mem->frame.handle(ACCESS_RW);//CL address
    }

    //g_print("ocl_memory_alloc: ocl_mem = %p, ocl_mem->mem=%p, size = %ld, address=%p\n",
    //      ocl_mem, ocl_mem->mem, ocl_mem->mem_size, ocl_mem->frame.getMat(0).ptr() );

    GstMemory *memory = GST_MEMORY_CAST (ocl_mem);
    gst_memory_init (memory, GST_MEMORY_FLAG_NO_SHARE, priv->allocator, NULL,
          GST_VIDEO_INFO_SIZE (&priv->info), 0, 0, GST_VIDEO_INFO_SIZE (&priv->info));

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

    // We can set other mem as the qdata of ocl_mem
    //gst_mini_object_set_qdata (GST_MINI_OBJECT_CAST (ocl_mem),
    //    OCL_MEMORY_QUARK, GST_MEMORY_CAST (other_mem),
    //    (GDestroyNotify) gst_memory_unref);

    gst_buffer_append_memory (ocl_buf, (GstMemory *)ocl_mem);

    GstVideoInfo *info = &priv->info;

    GST_DEBUG_OBJECT (pool,
        " %dx%d, sz: %zu, offset: %zu, %zu, stride: %d, %d",
        info->width, info->height, info->size,
        info->offset[0], info->offset[1], info->stride[0], info->stride[1]);

    // For resconvert, it will make mfxenc can parse this buffer
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

    // ref=2 ???
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
