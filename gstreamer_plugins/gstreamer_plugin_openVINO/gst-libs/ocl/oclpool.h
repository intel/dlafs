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
/*
#define IS_OCL_POOL(pool)\
    (!strncmp (((GstObject *) pool)->name, OCL_POOL_NAME, strlen (OCL_POOL_NAME)))
*/

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
