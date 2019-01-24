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
 * NV12 + RGBA
 * The same image size
 */

static __constant
float c_YUV2RGBCoeffs_420[5] =
{
     1.163999557f,
     2.017999649f,
    -0.390999794f,
    -0.812999725f,
     1.5959997177f
};

static const __constant float CV_8U_MAX         = 255.0f;
static const __constant float CV_8U_HALF        = 128.0f;
static const __constant float BT601_BLACK_RANGE = 16.0f;
static const __constant float CV_8U_SCALE       = 1.0f / 255.0f;
static const __constant float d1                = BT601_BLACK_RANGE / CV_8U_MAX;
static const __constant float d2                = CV_8U_HALF / CV_8U_MAX;


__kernel void blend( read_only image2d_t src_y,
                     read_only image2d_t src_uv,
                     __global unsigned char* src_osd,
                    __global unsigned char* pBGRA,
                     int dst_w, int dst_h)
{
    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int id_z = 2 * id_x;
    int id_w = 2 * id_y;
    float4 Y0, Y1, Y2, Y3, UV;

    if (id_z >= dst_w || id_w >= dst_h)
        return;

    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    Y0 = read_imagef(src_y, sampler, (int2)(id_z    , id_w));
    Y1 = read_imagef(src_y, sampler, (int2)(id_z + 1, id_w));
    Y2 = read_imagef(src_y, sampler, (int2)(id_z    , id_w + 1));
    Y3 = read_imagef(src_y, sampler, (int2)(id_z + 1, id_w + 1));
    UV = read_imagef(src_uv,sampler,(int2)(id_z/2  , id_w / 2)) - d2;

    __global uchar* pDstRow1 = pBGRA + (id_w * dst_w + id_z) * 4;
    __global uchar* pDstRow2 = pDstRow1 + dst_w * 4;

    __global uchar* pOsdRow1 = src_osd + (id_w * dst_w + id_z) * 4;
    __global uchar* pOsdRow2 = pOsdRow1 + dst_w * 4;

    __constant float* coeffs = c_YUV2RGBCoeffs_420;

    Y0 = max(0.f, Y0 - d1) * coeffs[0];
    Y1 = max(0.f, Y1 - d1) * coeffs[0];
    Y2 = max(0.f, Y2 - d1) * coeffs[0];
    Y3 = max(0.f, Y3 - d1) * coeffs[0];

    float ruv = fma(coeffs[4], UV.y, 0.0f);
    float guv = fma(coeffs[3], UV.y, fma(coeffs[2], UV.x, 0.0f));
    float buv = fma(coeffs[1], UV.x, 0.0f);


    // pOsdRow - RGBA
    float R0 = (Y0.x + ruv) * (255 - pOsdRow1[3]) + pOsdRow1[0];
    float G0 = (Y0.x + guv) * (255 - pOsdRow1[3]) + pOsdRow1[1];
    float B0 = (Y0.x + buv) * (255 - pOsdRow1[3]) + pOsdRow1[2];

    float R1 = (Y1.x + ruv) * (255 - pOsdRow1[7]) + pOsdRow1[4];
    float G1 = (Y1.x + guv) * (255 - pOsdRow1[7]) + pOsdRow1[5];
    float B1 = (Y1.x + buv) * (255 - pOsdRow1[7]) + pOsdRow1[6];

    float R2 = (Y2.x + ruv) * (255 - pOsdRow2[3]) + pOsdRow2[0];
    float G2 = (Y2.x + guv) * (255 - pOsdRow2[3]) + pOsdRow2[1];
    float B2 = (Y2.x + buv) * (255 - pOsdRow2[3]) + pOsdRow2[2];

    float R3 = (Y3.x + ruv) * (255 - pOsdRow2[7]) + pOsdRow2[4];
    float G3 = (Y3.x + guv) * (255 - pOsdRow2[7]) + pOsdRow2[5];
    float B3 = (Y3.x + buv) * (255 - pOsdRow2[7]) + pOsdRow2[6];


    pDstRow1[0] = convert_uchar_sat(B0);
    pDstRow1[1] = convert_uchar_sat(G0);
    pDstRow1[2] = convert_uchar_sat(R0);
    pDstRow1[3] = pOsdRow1[3];

    pDstRow1[4] = convert_uchar_sat(B1);
    pDstRow1[5] = convert_uchar_sat(G1);
    pDstRow1[6] = convert_uchar_sat(R1);
    pDstRow1[7] = pOsdRow1[7];

    pDstRow2[0] = convert_uchar_sat(B2);
    pDstRow2[1] = convert_uchar_sat(G2);
    pDstRow2[2] = convert_uchar_sat(R2);
    pDstRow2[3] = pOsdRow2[3];

    pDstRow2[4] = convert_uchar_sat(B3);
    pDstRow2[5] = convert_uchar_sat(G3);
    pDstRow2[6] = convert_uchar_sat(R3);
    pDstRow2[7] = pOsdRow2[7];
}
