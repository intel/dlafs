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

#ifndef VIDEO_COMMON_DEFS_H_
#define VIDEO_COMMON_DEFS_H_

#include <stdint.h>
#include <va/va.h>
#include <gst/gstbuffer.h>
#include <CL/cl.h>


#ifdef __cplusplus

#include <tr1/memory>
#include <tr1/functional>

namespace HDDLStreamFilter
{
#define Bind std::tr1::bind
#define SharedPtr std::tr1::shared_ptr
#define WeakPtr std::tr1::weak_ptr
#define DynamicPointerCast std::tr1::dynamic_pointer_cast
#define StaticPointerCast std::tr1::static_pointer_cast
#define EnableSharedFromThis std::tr1::enable_shared_from_this

#define SharedFromThis shared_from_this
#define SHARED_PTR_IS_VALID(ptr) (ptr.get() != NULL)

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(className) \
      className(const className&); \
      className & operator=(const className&);
#endif
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#ifndef bool
#define bool  int
#endif

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif
#endif

#define OCL_FOURCC(ch0, ch1, ch2, ch3) \
        ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
         ((uint32_t)(uint8_t)(ch2) << 16)  | ((uint32_t)(uint8_t)(ch3) << 24))

#define OCL_FOURCC_NV12 OCL_FOURCC('N','V','1','2')
#define OCL_FOURCC_BGRX OCL_FOURCC('B','G','R','X')
#define OCL_FOURCC_BGRA OCL_FOURCC('B','G','R','A')
#define OCL_FOURCC_BGR3 OCL_FOURCC('B','G','G','3')
#define OCL_FOURCC_BGRP OCL_FOURCC('B','G','G','P')
#define OCL_FOURCC_GRAY OCL_FOURCC('G','R','A','Y')


typedef enum {
    NATIVE_DISPLAY_AUTO,    // decided by ocl
    NATIVE_DISPLAY_VA,      /* client need init va*/
} OclNativeDisplayType;

#define INVALID_DISPLAY_FD     -1
#define INVALID_DISPLAY_HANDLE NULL

typedef struct {
    intptr_t handle;
    OclNativeDisplayType type;
} NativeDisplay;

typedef enum {
    VIDEO_DATA_MEMORY_TYPE_RAW_POINTER,  // pass data pointer to client
    VIDEO_DATA_MEMORY_TYPE_RAW_COPY,     // copy data to client provided buffer, renderDone() is not necessary
    VIDEO_DATA_MEMORY_TYPE_DRM_NAME,     // render output frame by egl/gles, connect with OpenCL
    VIDEO_DATA_MEMORY_TYPE_DMA_BUF,      // share buffer with camera device etc
    VIDEO_DATA_MEMORY_TYPE_SURFACE_ID,  // it can be used for surface sharing of transcoding, benefits suppressed rendering as well.
                                        //it is discouraged to use it for video rendering.
    VIDEO_DATA_MEMORY_TYPE_ANDROID_NATIVE_BUFFER, // ANativeWindowBuffer for android
} VideoDataMemoryType;

typedef struct {
    VideoDataMemoryType memoryType;
    uint32_t width;
    uint32_t height;
    uint32_t pitch[3];
    uint32_t offset[3];
    uint32_t fourcc;
    uint32_t size;
    intptr_t handle;        // planar data has one fd for now, raw data also uses one pointer (+ offset)
    uint32_t internalID; // internal identification for image/surface recycle
    int64_t  timeStamp;
    uint32_t flags;             //see VIDEO_FRAME_FLAGS_XXX
}VideoFrameRawData;

#define VIDEO_FRAME_FLAGS_KEY 1

typedef enum {
    OCL_SUCCESS = 0,
    OCL_MORE_DATA,
    OCL_NOT_IMPLEMENT,
    OCL_INVALID_PARAM,
    OCL_OUT_MEMORY,

    OCL_FATAL_ERROR = 128,
    OCL_FAIL,
    OCL_NO_CONFIG,
    OCL_DRIVER_FAIL,
} OclStatus;

typedef struct {
    //in
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;

    //in, out, number of surfaces
    //for decoder, this min surfaces size need for decoder
    //you should alloc extra size for perfomance or display queue
    //you have two chance to get this size
    // 1. from VideoFormatInfo.surfaceNumber, after you got DECODE_FORMAT_CHANGE
    // 2. in SurfaceAllocator::alloc
    uint32_t size;

    //out
    intptr_t* surfaces;
} SurfaceAllocParams;

typedef struct _SurfaceAllocator SurfaceAllocator;
//allocator for surfaces, ocl uses this to get
// 1. decoded surface for decoder.
// 2. reconstruct surface for encoder.
struct _SurfaceAllocator {
    void*      *user;   /* you can put your private data here, ocl will not touch it */
    /* alloc and free surfaces */
    OclStatus (*alloc) (SurfaceAllocator* thiz, SurfaceAllocParams* params);
    OclStatus (*free)  (SurfaceAllocator* thiz, SurfaceAllocParams* params);
    /* after this called, ocl will not use the allocator */
    void       (*unref) (SurfaceAllocator* thiz);
};

typedef struct {
    uint32_t  x;
    uint32_t  y;
    uint32_t  width;
    uint32_t  height;
} VideoRect;

typedef struct {
    int32_t x;
    int32_t y;
} VideoPoint;

#define MAX_TRAJECTORY_POINTS_NUM 128
typedef struct _InferenceData{
    float probility;
    VideoRect rect;
    char label[128];
    VideoPoint track[MAX_TRAJECTORY_POINTS_NUM];
    int   track_num;
}InferenceData;

typedef struct _OclGstMfxVideoMeta
{
  GstBuffer *buffer;
  gint ref_count;
  void *surface;

  /* check linear buffer */
  gboolean is_linear;
  guint token;
  VASurfaceID surface_id;
  VADisplay display_id;
}OclGstMfxVideoMeta;

typedef struct _OclGstMfxVideoMetaHolder
{
  GstMeta base;
  OclGstMfxVideoMeta *meta;
}OclGstMfxVideoMetaHolder;

typedef struct {
    cl_mem      mem;
    uint32_t    fourcc;
    uint32_t    width;
    uint32_t    height;

    VASurfaceID surface; /* usded for OCL input only */

    VideoRect   crop;
    uint32_t    flags;
    int64_t     timeStamp;

#ifdef __ENABLE_CAPI__
    /**
     * for frame destory, cpp should not touch here
     */
    intptr_t    user_data;
    void        (*free)(struct VideoFrame* );
#endif
} VideoFrame;

#define OCL_MIME_H264 "video/h264"
#define OCL_MIME_AVC  "video/avc"
#define OCL_MIME_H265 "video/h265"
#define OCL_MIME_HEVC "video/hevc"
#define OCL_MIME_JPEG "image/jpeg"

#define OCL_VPP_CRC           "vpp/ocl_crc"
#define OCL_VPP_BLENDER   "vpp/ocl_blender"

#ifdef __cplusplus
}
#endif

#endif                          // VIDEO_COMMON_DEFS_H_
