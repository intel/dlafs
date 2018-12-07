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
#include "config.h"
#endif
     
#include <string.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>
#include <gst/base/gstbytereader.h>
#include <memory.h>
#include "resmemory.h"
#include "common/macros.h"

GST_DEBUG_CATEGORY_STATIC (res_memory_debug);
#define GST_CAT_DEFAULT res_memory_debug

GQuark res_memory_quark (void)
{
    static GQuark quark = 0;

    if (!quark) {
        quark = g_quark_from_string ("GstResMemory");
    }

    return quark;
}

ResMemory*
res_memory_acquire (GstBuffer* buffer)
{
    GstMemory *memory = gst_buffer_peek_memory (buffer, 0);

    if (memory ) {
        ResMemory* res_mem = (ResMemory*)memory;
        if (res_mem &&
            !strcmp (GST_MEMORY_CAST(res_mem)->allocator->mem_type,
                RES_ALLOCATOR_NAME))
            return res_mem;
    }
    return NULL;
}

GstBuffer*
res_buffer_alloc (GstBufferPool* pool)
{
    if (!pool) {
        GST_ERROR ("invalid buffer pool");
        return NULL;
    }

    if (!gst_buffer_pool_set_active (pool, TRUE)) {
        GST_ERROR ("failed to activate buffer pool");
        return NULL;
    }

    GstBuffer *buffer = NULL;
    if (gst_buffer_pool_acquire_buffer (pool, &buffer, NULL) != GST_FLOW_OK) {
        GST_ERROR ("failed to create buffer");
        return NULL;
    }

    return buffer;
}

void res_memory_data_resize(GstMemory *memory, int count)
{
    if(count==0) {
        RES_MEMORY_DATA_COUNT(memory) = 0;
        g_free(RES_MEMORY_DATA(memory));
        RES_MEMORY_DATA(memory) = NULL;
        return;
    }

    if(RES_MEMORY_DATA_COUNT(memory) == count)
        return;

    if(RES_MEMORY_DATA_COUNT(memory)==0){
        RES_MEMORY_DATA(memory) = g_new0(InferenceData, count);
    } else {
        RES_MEMORY_DATA(memory) = g_try_renew(InferenceData, RES_MEMORY_DATA(memory), count);
    }
    RES_MEMORY_DATA_COUNT(memory) = count;
}


/* Called when gst_object_unref(memory) */
static void
res_memory_free (GstAllocator* allocator, GstMemory* memory)
{
    if (!allocator || !memory)
        return;

    //ResAllocator* res_alloc = RES_ALLOCATOR_CAST (allocator);
    InferenceData *data = RES_MEMORY_DATA(memory);
    //int count = RES_MEMORY_DATA_COUNT(memory);
    // g_print("res_memory_free: res_mem = %p, res_mem->data=%p\n",memory, data );
    if(data)
        g_free(data);
    RES_MEMORY_DATA(memory) = NULL;
    RES_MEMORY_DATA_COUNT(memory) = 0;

    g_free (memory);
}

static gpointer
res_memory_map (GstMemory* memory, gsize size, GstMapFlags flags)
{
    gpointer data = 0;
    ResMemory *res_mem = RES_MEMORY_CAST(memory);

    data = (gpointer)res_mem->data;
    return data;
}
static void
res_memory_unmap (GstMemory* memory)
{
    return;
}


G_DEFINE_TYPE (ResAllocator, res_allocator, GST_TYPE_ALLOCATOR);

static void
res_allocator_class_init (ResAllocatorClass* res_class)
{
    GstAllocatorClass* klass = (GstAllocatorClass*) res_class;

    klass->alloc = NULL;
    /* Called when gst_object_unref(memory) */
    klass->free = res_memory_free;

    GST_DEBUG_CATEGORY_INIT (res_memory_debug,
	      "res_memory", 0, "ResMemory and ResAllocator");
}

static void
res_allocator_init (ResAllocator* res_alloc)
{
    GstAllocator* allocator = GST_ALLOCATOR_CAST (res_alloc);

    allocator->mem_map = res_memory_map;
    allocator->mem_unmap = res_memory_unmap;
    allocator->mem_type = RES_ALLOCATOR_NAME;

    GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

GstAllocator*
res_allocator_new ()
{
    ResAllocator* res_alloc =
        RES_ALLOCATOR_CAST (g_object_new (RES_ALLOCATOR_TYPE, NULL));

    return GST_ALLOCATOR_CAST (res_alloc);
}
