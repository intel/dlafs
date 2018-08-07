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

#ifndef __RES_POOL_H__
#define __RES_POOL_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include "resmemory.h"

G_BEGIN_DECLS

typedef struct _ResPool        ResPool;
typedef struct _ResPoolClass   ResPoolClass;
typedef struct _ResPoolPrivate ResPoolPrivate;

#define GST_TYPE_RES_POOL      (res_pool_get_type())
#define RES_POOL_CAST(obj)     ((ResPool*)(obj))

#define RES_POOL_NAME "respool"
#define IS_RES_POOL(pool)\
    (!strncmp (((GstObject *) pool)->name, RES_POOL_NAME, strlen (RES_POOL_NAME)))

#define RES_POOL_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_RES_POOL, ResPoolPrivate))

typedef GstMemory* (*MemoryAlloc) (ResPool* respool);

struct _ResPool
{
    GstBufferPool bufferpool;
    ResPoolPrivate *priv;
    MemoryAlloc memory_alloc;
};

struct _ResPoolClass
{
    GstBufferPoolClass parent_class;
};

GType res_pool_get_type (void);

GstBufferPool* res_pool_create (GstCaps* caps, gsize size, gint min, gint max);

G_END_DECLS

#endif
