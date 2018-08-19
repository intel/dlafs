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

#ifndef __RES_MEMORY_H__
#define __RES_MEMORY_H__

#include <gst/gst.h>
//#include "common/lock.h"
#include <interface/videodefs.h>

G_BEGIN_DECLS

typedef struct _ResMemory         ResMemory;
typedef struct _ResAllocator      ResAllocator;
typedef struct _ResAllocatorClass ResAllocatorClass;

struct _ResMemory {
    GstMemory   parent;
    InferenceData *data;
    int         data_count;
    guint64 pts;
};

struct _ResAllocator {
    GstAllocator parent;
};

struct _ResAllocatorClass {
    GstAllocatorClass parent_class;
};

#define RES_MEMORY_CAST(memory)    ((ResMemory*)memory)
#define RES_MEMORY_DATA(memory)    (RES_MEMORY_CAST (memory)->data)
#define RES_MEMORY_DATA_COUNT(memory)    (RES_MEMORY_CAST (memory)->data_count)
#define RES_MEMORY_PTS(memory)     (RES_MEMORY_CAST (memory)->pts)

#define RES_MEMORY_QUARK            res_memory_quark()


#define RES_ALLOCATOR_NAME            "res_allocator"
#define RES_ALLOCATOR_CAST(obj)       ((ResAllocator*)(obj))
#define RES_ALLOCATOR_TYPE            (res_allocator_get_type())
#define IS_RES_ALLOCATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RES_ALLOCATOR_TYPE))
#define IS_RES_ALLOCATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RES_ALLOCATOR_TYPE))
#define RES_ALLOCATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RES_ALLOCATOR_TYPE, ResAllocator))
#define RES_ALLOCATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RES_ALLOCATOR_TYPE, ResAllocatorClass))
#define RES_ALLOCATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RES_ALLOCATOR_TYPE, ResAllocatorClass))

GType res_allocator_get_type (void);
GQuark res_memory_quark (void);

ResMemory* res_memory_acquire (GstBuffer* buffer);
GstBuffer* res_buffer_alloc (GstBufferPool* pool);
GstAllocator* res_allocator_new (void);

void res_memory_data_resize(GstMemory *memory, int count);

G_END_DECLS

#endif
