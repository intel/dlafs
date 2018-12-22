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


#ifndef __BLEND_WRAPPER_H__
#define __BLEND_WRAPPER_H__

#include <gst/gstbuffer.h>
#include <gst/gstpad.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _cvdl_blender{
    GstBufferPool* mOsdPool;
    void *mImgProcessor;
    int mImageWidth;
    int mImageHeight;
    gboolean mInited;
}CvdlBlender;

typedef CvdlBlender* BlendHandle;


BlendHandle blender_create();
void blender_destroy(BlendHandle handle);
void blender_init(BlendHandle handle, GstCaps* caps);
GstBuffer* blender_process_cvdl_buffer(BlendHandle handle, GstBuffer* buffer);


#ifdef __cplusplus
};
#endif
#endif
