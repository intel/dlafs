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

#ifndef __OCL_POOL_H__
#define __OCL_POOL_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include "oclmemory.h"

G_BEGIN_DECLS

typedef struct _OclPool        OclPool;
typedef struct _OclPoolClass   OclPoolClass;
typedef struct _OclPoolPrivate OclPoolPrivate;

#define GST_TYPE_OCL_POOL      (ocl_pool_get_type())
#define OCL_POOL_CAST(obj)     ((OclPool*)(obj))

#define OCL_POOL_NAME "oclpool"
#define IS_OCL_POOL(pool)\
    (!strncmp (((GstObject *) pool)->name, OCL_POOL_NAME, strlen (OCL_POOL_NAME)))

#define OCL_POOL_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_OCL_POOL, OclPoolPrivate))

typedef GstMemory* (*MemoryAlloc) (OclPool* oclpool);

struct _OclPool
{
    GstBufferPool bufferpool;
    OclPoolPrivate *priv;
    MemoryAlloc memory_alloc;
};

struct _OclPoolClass
{
    GstBufferPoolClass parent_class;
};

GType ocl_pool_get_type (void);

GstBufferPool* ocl_pool_create (GstCaps* caps, gsize size, gint min, gint max);

G_END_DECLS

#endif
