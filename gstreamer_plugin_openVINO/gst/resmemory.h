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
