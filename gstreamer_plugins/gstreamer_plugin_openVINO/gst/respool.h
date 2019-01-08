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
/*
#define IS_RES_POOL(pool)\
    (!strncmp (((GstObject *) pool)->name, RES_POOL_NAME, strlen (RES_POOL_NAME)))
*/
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
