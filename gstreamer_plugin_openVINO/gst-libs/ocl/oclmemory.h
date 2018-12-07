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

#ifndef __OCL_MEMORY_H__
#define __OCL_MEMORY_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include "va/va.h"
#include "common/lock.h"
#include <vector>
#include <opencv2/opencv.hpp>
#include <va/va.h>
#include <CL/cl.h>


G_BEGIN_DECLS

typedef struct _OclMemory         OclMemory;
typedef struct _OclAllocator      OclAllocator;
typedef struct _OclAllocatorClass OclAllocatorClass;

enum OclMemoryType
{
    OCL_MEMORY_VA,
    OCL_MEMORY_OCV,
    OCL_MEMORY_NUM
};

struct _OclMemory {
    GstMemory   parent;

    cv::UMat    frame;
    guint       fourcc;
    cl_mem      mem; //(cl_mem)frame.handle(ACCESS_WRITE);
    gint        mapcount;
    gint        x;
    gint        y;
    gint        width;
    gint        height;
    size_t      size;
    size_t    mem_size;
    gint        purpose;

    HDDLStreamFilter::Lock        lock;
};

struct _OclAllocator {
    GstAllocator parent;

    OclMemoryType mem_type;
};

struct _OclAllocatorClass {
    GstAllocatorClass parent_class;
};

#define OCL_MEMORY_CAST(memory)       ((OclMemory*)memory)
#define OCL_MEMORY_SIZE(memory)       (OCL_MEMORY_CAST (memory)->size)
#define OCL_MEMORY_WIDTH(memory)      (OCL_MEMORY_CAST (memory)->width)
#define OCL_MEMORY_HEIGHT(memory)     (OCL_MEMORY_CAST (memory)->height)
#define OCL_MEMORY_LOCK(memory)       (OCL_MEMORY_CAST (memory)->lock)
#define OCL_MEMORY_MEM(memory)        (OCL_MEMORY_CAST (memory)->mem)
#define OCL_MEMORY_FOURCC(memory)     (OCL_MEMORY_CAST (memory)->fourcc)
#define OCL_MEMORY_FRAME(memory)      (OCL_MEMORY_CAST (memory)->frame)
#define OCL_MEMORY_MAPCOUNT(memory)   (OCL_MEMORY_CAST (memory)->mapcount)
#define OCL_MEMORY_QUARK              ocl_memory_quark()


#define OCL_ALLOCATOR_NAME            "ocl_allocator"
#define OCL_ALLOCATOR_CAST(obj)       ((OclAllocator*)(obj))
#define OCL_ALLOCATOR_TYPE            (ocl_allocator_get_type())
#define IS_OCL_ALLOCATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OCL_ALLOCATOR_TYPE))
#define IS_OCL_ALLOCATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  OCL_ALLOCATOR_TYPE))
#define OCL_ALLOCATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OCL_ALLOCATOR_TYPE, OclAllocator))
#define OCL_ALLOCATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  OCL_ALLOCATOR_TYPE, OclAllocatorClass))
#define OCL_ALLOCATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  OCL_ALLOCATOR_TYPE, OclAllocatorClass))

GQuark
ocl_memory_quark (void);

OclMemory*
ocl_memory_acquire (GstBuffer* buffer);

GstBuffer*
ocl_buffer_alloc (GstBufferPool* pool);

gboolean
ocl_buffer_copy (GstBuffer* src, GstBuffer* dst, GstVideoInfo* info);

void
ocl_memory_free (GstAllocator* allocator, GstMemory* memory);

GType
ocl_allocator_get_type (void);

GstAllocator*
ocl_allocator_new (OclMemoryType mem_type);

G_END_DECLS

#endif
