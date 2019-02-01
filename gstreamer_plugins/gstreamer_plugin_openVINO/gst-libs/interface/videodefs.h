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


#ifndef VIDEO_COMMON_DEFS_H_
#define VIDEO_COMMON_DEFS_H_

#include <stdint.h>
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
#define OCL_FOURCC_YV12 OCL_FOURCC('Y','V','1','2')
#define OCL_FOURCC_I420 OCL_FOURCC('I','4','2','0')
#define OCL_FOURCC_YUY2 OCL_FOURCC('Y','U','Y','2')
#define OCL_FOURCC_UYVY OCL_FOURCC('U','Y','V','Y')


#ifdef __WIN32__
    #include <d3d11.h>
    typedef ID3D11Texture2D VideoSurfaceID;
    typedef *ID3D11Device   VideoDisplayID;
    #define INVALID_SURFACE_ID (-1)
#else//linux
    #include <va/va.h>
    typedef VASurfaceID  VideoSurfaceID;
    typedef VADisplay    VideoDisplayID;
    #define INVALID_SURFACE_ID VA_INVALID_SURFACE
#endif


typedef enum {
    OCL_SUCCESS = 0,
    OCL_INVALID_PARAM,
    OCL_FAIL,
} OclStatus;

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
    int track_num;
    // frame index of this inference data belong to
    int frame_index;
}InferenceData;

typedef struct _OclGstMfxVideoMeta
{
  GstBuffer *buffer;
  gint ref_count;
  void *surface;

  /* check linear buffer */
  gboolean is_linear;
  guint token;
  VideoSurfaceID surface_id;
  VideoDisplayID display_id;
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

    VideoSurfaceID surface; /* usded for OCL input only */

    VideoRect   crop;
    uint32_t    flags;
    int64_t     timeStamp;
} VideoFrame;

enum CRCFormat{
    CRC_FORMAT_BGR = 0,
    CRC_FORMAT_BGR_PLANNAR = 1,
    CRC_FORMAT_GRAY = 2,
};

typedef enum {
    VPP_CRC_PARAM,
    VPP_BLEND_PARAM,
} VppParamType;

typedef struct {
    VppParamType type;
    guint32 src_w;
    guint32 src_h;
    guint32 crop_x;
    guint32 crop_y;
    guint32 crop_w;
    guint32 crop_h;
    guint32 dst_w;
    guint32 dst_h;
} VppCrcParam;

typedef struct {
    VppParamType type;
    guint32 x;
    guint32 y;
    guint32 w;
    guint32 h;
} VppBlendParam;


#define OCL_MIME_H264 "video/h264"
#define OCL_MIME_AVC  "video/avc"
#define OCL_MIME_H265 "video/h265"
#define OCL_MIME_HEVC "video/hevc"
#define OCL_MIME_JPEG "image/jpeg"

#define OCL_VPP_CRC       "vpp/ocl_crc"
#define OCL_VPP_BLENDER   "vpp/ocl_blender"

#ifdef __cplusplus
}
#endif

#endif                          // VIDEO_COMMON_DEFS_H_
