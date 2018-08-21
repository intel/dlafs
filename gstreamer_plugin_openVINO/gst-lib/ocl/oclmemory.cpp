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

#include <memory.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <va/va_drmcommon.h>
#include <gst/allocators/gstdmabuf.h>

#include "oclmemory.h"
#include "ocl/oclutils.h"
#include "common/macros.h"
#include <CL/cl.h>

using namespace cv;

GST_DEBUG_CATEGORY_STATIC (ocl_memory_debug);
#define GST_CAT_DEFAULT ocl_memory_debug

GQuark
ocl_memory_quark (void)
{
    static GQuark quark = 0;

    if (!quark) {
        quark = g_quark_from_string ("GstOclMemory");
    }

    return quark;
}

OclMemory*
ocl_memory_acquire (GstBuffer* buffer)
{
    GstMemory *memory = gst_buffer_peek_memory (buffer, 0);

    if (memory ) {
        OclMemory* ocl_mem = (OclMemory*)
            gst_mini_object_get_qdata (GST_MINI_OBJECT (memory), OCL_MEMORY_QUARK);

        if (ocl_mem &&
            !strcmp (GST_MEMORY_CAST(ocl_mem)->allocator->mem_type, OCL_ALLOCATOR_NAME))
            return ocl_mem;
    }

    return NULL;
}

GstBuffer*
ocl_buffer_alloc (GstBufferPool* pool)
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

gboolean
ocl_buffer_copy (GstBuffer* src, GstBuffer* dst, GstVideoInfo* info)
{
    GstVideoFrame src_frame, dst_frame;

    if (!src || !dst || !info)
        return FALSE;

    if (!gst_video_frame_map (&src_frame, info, src, GST_MAP_READ)) {
        GST_WARNING ("failed to map src buffer");
        return FALSE;
    }

    if (!gst_video_frame_map (&dst_frame, info, dst, GST_MAP_WRITE)) {
        GST_WARNING ("failed to map dst buffer");
        gst_video_frame_unmap (&src_frame);
        return FALSE;
    }

    if (GST_VIDEO_FRAME_WIDTH (&src_frame) > GST_VIDEO_FRAME_WIDTH (&dst_frame))
        GST_VIDEO_FRAME_WIDTH (&src_frame) = GST_VIDEO_FRAME_WIDTH (&dst_frame);
    else
        GST_VIDEO_FRAME_WIDTH (&dst_frame) = GST_VIDEO_FRAME_WIDTH (&src_frame);

    if (GST_VIDEO_FRAME_HEIGHT (&src_frame) > GST_VIDEO_FRAME_HEIGHT (&dst_frame))
        GST_VIDEO_FRAME_HEIGHT (&src_frame) = GST_VIDEO_FRAME_HEIGHT (&dst_frame);
    else
        GST_VIDEO_FRAME_HEIGHT (&dst_frame) = GST_VIDEO_FRAME_HEIGHT (&src_frame);

    gboolean success = gst_video_frame_copy (&dst_frame, &src_frame);

    gst_video_frame_unmap (&src_frame);
    gst_video_frame_unmap (&dst_frame);

    if (success) {
        gst_buffer_copy_into (dst, src, GST_BUFFER_COPY_TIMESTAMPS, 0, -1);
    } else {
        GST_ERROR ("failed to copy video frame");
    }

    return success;
}

void
ocl_memory_free (GstAllocator* allocator, GstMemory* memory)
{
    if (!allocator || !memory)
        return;

    OclMemory *ocl_mem = OCL_MEMORY_CAST(memory);
    OclAllocator* ocl_alloc = OCL_ALLOCATOR_CAST (allocator);
    if(ocl_alloc->mem_type != OCL_MEMORY_OCV){
        GST_ERROR("Error in ocl_memory_free(): mem_type is %d\n", ocl_alloc->mem_type);
    }

    // release cv::UMAT
    ocl_mem->frame.release();
    ocl_mem->mem = 0;
    g_free (memory);
}

static gpointer
ocl_memory_map (GstMemory* memory, gsize size, GstMapFlags flags)
{
    HDDLStreamFilter::AutoLock l(OCL_MEMORY_LOCK (memory));
    gpointer data = 0;
    OclMemory *ocl_mem = OCL_MEMORY_CAST(memory);

    //if (!OCL_MEMORY_MAPCOUNT (memory)) {
    //    OCL_MEMORY_MEM (memory) = (cl_mem)OCL_MEMORY_FRAME(memory).handle(ACCESS_RW);
    //}
    ++OCL_MEMORY_MAPCOUNT (memory);

    /*map ocl memory into system memory, which is used by mfxjpegdec to copy input data into video surface
       * call stack:
       *     #0  ocl_memory_map
       *     #1  gst_memory_map
       *     #2  gst_memory_make_mapped
       *     #3  gst_buffer_map_range
       *     #4  default_map
       *     #5  gst_video_frame_map_id
       *     #6  gst_mfx_plugin_base_get_input_buffer
       *     #7  gst_mfxenc_handle_frame
       *     #8  gst_video_encoder_chain
       *     #9  gst_pad_chain_data_unchecked
       *     #10 gst_pad_push_data
       *     #11 gst_pad_push
       *     #12 res_convert_send_data
       *     #13 res_convert_chain
       */
    //data = OCL_MEMORY_MEM (memory);
    data =  (gpointer)ocl_mem->frame.getMat(0).ptr();

    return data;
}

static void
ocl_memory_unmap (GstMemory* memory)
{
    if (!memory)
        return;

    HDDLStreamFilter::AutoLock l(OCL_MEMORY_LOCK (memory));

    if (!OCL_MEMORY_MAPCOUNT (memory)) {
        GST_ERROR("double free for %p", memory);
        return;
    }

    --OCL_MEMORY_MAPCOUNT (memory);
}

G_DEFINE_TYPE (OclAllocator, ocl_allocator, GST_TYPE_ALLOCATOR);

static void
ocl_allocator_class_init (OclAllocatorClass* ocl_class)
{
    GstAllocatorClass* klass = (GstAllocatorClass*) ocl_class;

    klass->alloc = NULL;
    klass->free = ocl_memory_free;

    GST_DEBUG_CATEGORY_INIT (ocl_memory_debug,
	      "ocl_memory", 0, "OclMemory and OclAllocator");
}

static void
ocl_allocator_init (OclAllocator* ocl_alloc)
{
    GstAllocator* allocator = GST_ALLOCATOR_CAST (ocl_alloc);

    allocator->mem_map = ocl_memory_map;
    allocator->mem_unmap = ocl_memory_unmap;
    allocator->mem_type = OCL_ALLOCATOR_NAME;

    GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

GstAllocator*
ocl_allocator_new (OclMemoryType mem_type)
{
    OclAllocator* ocl_alloc =
        OCL_ALLOCATOR_CAST (g_object_new (OCL_ALLOCATOR_TYPE, NULL));

    ocl_alloc->mem_type = mem_type;

    return GST_ALLOCATOR_CAST (ocl_alloc);
}


