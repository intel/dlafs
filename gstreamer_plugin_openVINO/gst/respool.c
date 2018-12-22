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


#include <unistd.h>
#include <string.h>
#include "respool.h"

struct _ResPoolPrivate
{
    GstCaps *caps;

    GstAllocator *allocator;
    GstAllocationParams params;
};


#define GST_CAT_DEFAULT res_pool_debug
GST_DEBUG_CATEGORY_STATIC (res_pool_debug);

#define res_pool_parent_class parent_class
G_DEFINE_TYPE (ResPool, res_pool, GST_TYPE_BUFFER_POOL);


static gboolean
res_pool_set_config (GstBufferPool* pool, GstStructure* config)
{
    GstCaps *caps = NULL;
    guint size, min, max;

    if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min, &max)) {
        GST_WARNING_OBJECT (pool, "invalid config");
        return FALSE;
    }

    //ResPoolPrivate *priv = RES_POOL_CAST (pool)->priv;
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

    return GST_BUFFER_POOL_CLASS (parent_class)->set_config (pool, config);
}


static GstMemory*
res_memory_alloc (ResPool* respool)
{
    ResPoolPrivate *priv = respool->priv;
    ResMemory *res_mem = g_new0(ResMemory, 1);
    GstMemory *parent;

    res_mem->data = g_new0(InferenceData, 1);
    res_mem->data_count = 1;
    res_mem->pts  = 0;
    //g_print("res_memory_alloc: res_mem = %p, res_mem->data=%p\n",res_mem, res_mem->data );

    /* find the real parent */
    if ((parent = res_mem->parent.parent) == NULL)
      parent = (GstMemory *) res_mem;

    //Only for gst-1.8.3,  but not sure if be removed when gst-1.14.2
    //It is used to avoid res_mem to be ref twice in gst_memory_init
    parent = NULL;

    GstMemory *memory = GST_MEMORY_CAST (res_mem);
    gst_memory_init (memory, GST_MEMORY_FLAG_NO_SHARE, priv->allocator, parent,
         sizeof(InferenceData), 0, 0, sizeof(InferenceData)); //TODO

    return memory;
}

static GstFlowReturn
res_pool_alloc (GstBufferPool* pool, GstBuffer** buffer,
    GstBufferPoolAcquireParams* params)
{
    GstBuffer *res_buf;
    ResMemory *res_mem;

    ResPool *respool = RES_POOL_CAST (pool);
    //ResPoolPrivate *priv = respool->priv;

    res_buf = gst_buffer_new ();
    res_mem = (ResMemory *) respool->memory_alloc (respool);
    if (!res_mem) {
        gst_buffer_unref (res_buf);
        GST_ERROR_OBJECT (pool, "failed to alloc res memory");
        return GST_FLOW_ERROR;
    }

    //TODO: GDestroyNotify need to release InferenceData?
   // gst_mini_object_set_qdata (GST_MINI_OBJECT_CAST (res_mem),
   //     RES_MEMORY_QUARK, GST_MEMORY_CAST (res_mem),
   //     (GDestroyNotify) gst_memory_unref);

    gst_buffer_append_memory (res_buf, (GstMemory *)res_mem);

    *buffer = res_buf;
    return GST_FLOW_OK;
}

static void
res_pool_finalize (GObject* object)
{
    ResPool *pool = RES_POOL_CAST (object);
    ResPoolPrivate *priv = pool->priv;

    GST_LOG_OBJECT (pool, "finalize res pool %p", pool);

    if (priv->caps)
        gst_caps_unref (priv->caps);

    if (priv->allocator)
        gst_object_unref (priv->allocator);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
res_pool_class_init (ResPoolClass* klass)
{
    GObjectClass *resclass = (GObjectClass *) klass;
    GstBufferPoolClass *pclass = (GstBufferPoolClass *) klass;

    g_type_class_add_private (klass, sizeof (ResPoolPrivate));

    resclass->finalize = res_pool_finalize;

    //gstbufferpool_class->get_options = res_pool_get_options;
    pclass->set_config = res_pool_set_config;
    pclass->alloc_buffer = res_pool_alloc;

    GST_DEBUG_CATEGORY_INIT (res_pool_debug, "res_pool", 0, "res_pool object");
}

static void
res_pool_init (ResPool* pool)
{
     pool->priv = RES_POOL_GET_PRIVATE (pool);
}


static GstBufferPool*
res_pool_new ()
{
    ResPool *pool;
    pool = (ResPool*) g_object_new (GST_TYPE_RES_POOL, NULL);

    pool->priv->allocator = res_allocator_new ();

    return GST_BUFFER_POOL_CAST (pool);
}

GstBufferPool*
res_pool_create (GstCaps* caps, gsize size, gint min, gint max)
{
    GstBufferPool *pool = res_pool_new ();

    RES_POOL_CAST(pool)->memory_alloc = res_memory_alloc;

    GstStructure *config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

    if (!gst_buffer_pool_set_config (pool, config)) {
        gst_object_unref (pool);
        pool = NULL;
    }

    return pool;
}
