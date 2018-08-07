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

#ifndef __BLEND_WRAPPER_H__
#define __BLEND_WRAPPER_H__

#include <gst/gstbuffer.h>
#include <gst/gstpad.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _cvdl_blender{
    GstBufferPool* mOsdPool;
    //ImageProcessor* mImgProcessor;
    void *mImgProcessor;
    int mImageWidth;
    int mImageHeight;
    gboolean mInited;
}CvdlBlender;

typedef CvdlBlender* BlendHandle;


BlendHandle blender_create();
void blender_destroy(BlendHandle handle);
void blender_init(BlendHandle handle, GstPad* pad);
GstFlowReturn blender_process_cvdl_buffer(BlendHandle handle, GstBuffer* buffer);


#ifdef __cplusplus
};
#endif
#endif
