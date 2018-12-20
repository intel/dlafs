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


#ifndef _VPP_DEFS_H_
#define _VPP_DEFS_H_

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif

#endif /*  _VPP_DEFS_H_ */
