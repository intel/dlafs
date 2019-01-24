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
  Crop -> Resize -> CSC
  CSC: NV12->BRG_Planar
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

#if 0
//test

__kernel
void crop_resize_csc_planar(
                __read_only image2d_t img_y_src,
                __read_only image2d_t img_uv_src,
                uint src_w, uint src_h,
                uint crop_x, uint crop_y,
                uint crop_w, uint crop_h,
                __global unsigned char* pBGR,
                uint dst_w, uint dst_h)
{
    int id_x = get_global_id(0);
    int id_y = get_global_id(1);

    int dst_x = 2 * id_x;
    int dst_y = 2 * id_y;

	if(dst_x >= dst_w || dst_y>= dst_h)
		return;

    sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE  |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_LINEAR;
    float dst_x0 = dst_x;
    float dst_x1 = dst_x + 1;
    float dst_y0 = dst_y;
    float dst_y1 = dst_y + 1;

    float x0 = (crop_x + dst_x0 * crop_w / dst_w) / (1.0 * src_w);
    float y0 = (crop_y + dst_y0 * crop_h / dst_h) / (1.0 * src_h);
    float x1 = (crop_x + dst_x1 * crop_w / dst_w) / (1.0 * src_w);
    float y1 = (crop_y + dst_y0 * crop_h / dst_h) / (1.0 * src_h);
    float x2 = (crop_x + dst_x0 * crop_w / dst_w) / (1.0 * src_w);
    float y2 = (crop_y + dst_y1 * crop_h / dst_h) / (1.0 * src_h);
    float x3 = (crop_x + dst_x1 * crop_w / dst_w) / (1.0 * src_w);
    float y3 = (crop_y + dst_y1 * crop_h / dst_h) / (1.0 * src_h);

    float4  Y0 = read_imagef (img_y_src, sampler, (float2)(x0, y0));
    float4  Y1 = read_imagef (img_y_src, sampler, (float2)(x1, y1));
    float4  Y2 = read_imagef (img_y_src, sampler, (float2)(x2, y2));
    float4  Y3 = read_imagef (img_y_src, sampler, (float2)(x3, y3));
    float4  UV = read_imagef (img_uv_src, sampler,(float2)(x0, y0)) - d2;

    __constant float* coeffs = c_YUV2RGBCoeffs_420;

    Y0 = max(0.f, Y0 - d1) * coeffs[0];
    Y1 = max(0.f, Y1 - d1) * coeffs[0];
    Y2 = max(0.f, Y2 - d1) * coeffs[0];
    Y3 = max(0.f, Y3 - d1) * coeffs[0];

    float ruv = fma(coeffs[4], UV.y, 0.0f);
    float guv = fma(coeffs[3], UV.y, fma(coeffs[2], UV.x, 0.0f));
    float buv = fma(coeffs[1], UV.x, 0.0f);

    float R1 = (Y0.x + ruv) * CV_8U_MAX;
    float G1 = (Y0.x + guv) * CV_8U_MAX;
    float B1 = (Y0.x + buv) * CV_8U_MAX;

    float R2 = (Y1.x + ruv) * CV_8U_MAX;
    float G2 = (Y1.x + guv) * CV_8U_MAX;
    float B2 = (Y1.x + buv) * CV_8U_MAX;

    float R3 = (Y2.x + ruv) * CV_8U_MAX;
    float G3 = (Y2.x + guv) * CV_8U_MAX;
    float B3 = (Y2.x + buv) * CV_8U_MAX;

    float R4 = (Y3.x + ruv) * CV_8U_MAX;
    float G4 = (Y3.x + guv) * CV_8U_MAX;
    float B4 = (Y3.x + buv) * CV_8U_MAX;

    __global uchar* pDstRow1 = pBGR + (dst_y * dst_w + dst_x) * 3;
    __global uchar* pDstRow2 = pDstRow1 + dst_w * 3;

    pDstRow1[0] = convert_uchar_sat(B1);
    pDstRow1[1] = convert_uchar_sat(G1);
    pDstRow1[2] = convert_uchar_sat(R1);
    pDstRow1[3] = convert_uchar_sat(B2);
    pDstRow1[4] = convert_uchar_sat(G2);
    pDstRow1[5] = convert_uchar_sat(R2);

    pDstRow2[0] = convert_uchar_sat(B3);
    pDstRow2[1] = convert_uchar_sat(G3);
    pDstRow2[2] = convert_uchar_sat(R3);
    pDstRow2[3] = convert_uchar_sat(B4);
    pDstRow2[4] = convert_uchar_sat(G4);
    pDstRow2[5] = convert_uchar_sat(R4);
}
#else

__kernel
void crop_resize_csc_planar(
                __read_only image2d_t img_y_src,
                __read_only image2d_t img_uv_src,
                uint src_w, uint src_h,
                uint crop_x, uint crop_y,
                uint crop_w, uint crop_h,
                __global unsigned char* pBGR,
                uint dst_w, uint dst_h)
{
    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int dst_x = 2 * id_x;
    int dst_y = 2 * id_y;

    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_LINEAR;

    int dst_x0 = dst_x;
    int dst_x1 = dst_x + 1;
    int dst_y0 = dst_y;
    int dst_y1 = dst_y + 1;

    int x0 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y0 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x1 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y1 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x2 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y2 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;
    int x3 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y3 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;

    float4  Y0 = read_imagef (img_y_src, sampler, (int2)(x0, y0));
    float4  Y1 = read_imagef (img_y_src, sampler, (int2)(x1, y1));
    float4  Y2 = read_imagef (img_y_src, sampler, (int2)(x2, y2));
    float4  Y3 = read_imagef (img_y_src, sampler, (int2)(x3, y3));
    float4  UV = read_imagef (img_uv_src, sampler,(int2)(x0/2, y0/2)) - d2;

    __constant float* coeffs = c_YUV2RGBCoeffs_420;

    Y0 = max(0.f, Y0 - d1) * coeffs[0];
    Y1 = max(0.f, Y1 - d1) * coeffs[0];
    Y2 = max(0.f, Y2 - d1) * coeffs[0];
    Y3 = max(0.f, Y3 - d1) * coeffs[0];

    float ruv = fma(coeffs[4], UV.y, 0.0f);
    float guv = fma(coeffs[3], UV.y, fma(coeffs[2], UV.x, 0.0f));
    float buv = fma(coeffs[1], UV.x, 0.0f);

    float R1 = (Y0.x + ruv) * CV_8U_MAX;
    float G1 = (Y0.x + guv) * CV_8U_MAX;
    float B1 = (Y0.x + buv) * CV_8U_MAX;

    float R2 = (Y1.x + ruv) * CV_8U_MAX;
    float G2 = (Y1.x + guv) * CV_8U_MAX;
    float B2 = (Y1.x + buv) * CV_8U_MAX;

    float R3 = (Y2.x + ruv) * CV_8U_MAX;
    float G3 = (Y2.x + guv) * CV_8U_MAX;
    float B3 = (Y2.x + buv) * CV_8U_MAX;

    float R4 = (Y3.x + ruv) * CV_8U_MAX;
    float G4 = (Y3.x + guv) * CV_8U_MAX;
    float B4 = (Y3.x + buv) * CV_8U_MAX;

    int plane_step = dst_w * dst_h;
    __global uchar* pDstRow1B = pBGR + (dst_y * dst_w + dst_x);
    __global uchar* pDstRow1G = pDstRow1B + plane_step;
    __global uchar* pDstRow1R = pDstRow1G + plane_step;

    __global uchar* pDstRow2B = pDstRow1B + dst_w;
    __global uchar* pDstRow2G = pDstRow1G + dst_w;
    __global uchar* pDstRow2R = pDstRow1R + dst_w;

    pDstRow1B[0] = convert_uchar_sat(B1);
    pDstRow1B[1] = convert_uchar_sat(B2);
    pDstRow2B[0] = convert_uchar_sat(B3);
    pDstRow2B[1] = convert_uchar_sat(B4);

    pDstRow1G[0] = convert_uchar_sat(G1);
    pDstRow1R[1] = convert_uchar_sat(R2);
    pDstRow2R[0] = convert_uchar_sat(R3);
    pDstRow2R[1] = convert_uchar_sat(R4);

    pDstRow1R[0] = convert_uchar_sat(R1);
    pDstRow1G[1] = convert_uchar_sat(G2);
    pDstRow2G[0] = convert_uchar_sat(G3);
    pDstRow2G[1] = convert_uchar_sat(G4);
}
#endif

__kernel
void crop_resize_csc(
                __read_only image2d_t img_y_src,
                __read_only image2d_t img_uv_src,
                uint src_w, uint src_h,
                uint crop_x, uint crop_y,
                uint crop_w, uint crop_h,
                __global unsigned char* pBGR,
                uint dst_w, uint dst_h)
{
    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int dst_x = 2 * id_x;
    int dst_y = 2 * id_y;

    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_LINEAR;

    int dst_x0 = dst_x;
    int dst_x1 = dst_x + 1;
    int dst_y0 = dst_y;
    int dst_y1 = dst_y + 1;

    int x0 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y0 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x1 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y1 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x2 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y2 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;
    int x3 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y3 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;

    float4  Y0 = read_imagef (img_y_src, sampler, (int2)(x0, y0));
    float4  Y1 = read_imagef (img_y_src, sampler, (int2)(x1, y1));
    float4  Y2 = read_imagef (img_y_src, sampler, (int2)(x2, y2));
    float4  Y3 = read_imagef (img_y_src, sampler, (int2)(x3, y3));
    float4  UV = read_imagef (img_uv_src, sampler,(int2)(x0/2, y0/2)) - d2;

    __constant float* coeffs = c_YUV2RGBCoeffs_420;

    Y0 = max(0.f, Y0 - d1) * coeffs[0];
    Y1 = max(0.f, Y1 - d1) * coeffs[0];
    Y2 = max(0.f, Y2 - d1) * coeffs[0];
    Y3 = max(0.f, Y3 - d1) * coeffs[0];

    float ruv = fma(coeffs[4], UV.y, 0.0f);
    float guv = fma(coeffs[3], UV.y, fma(coeffs[2], UV.x, 0.0f));
    float buv = fma(coeffs[1], UV.x, 0.0f);

    float R1 = (Y0.x + ruv) * CV_8U_MAX;
    float G1 = (Y0.x + guv) * CV_8U_MAX;
    float B1 = (Y0.x + buv) * CV_8U_MAX;

    float R2 = (Y1.x + ruv) * CV_8U_MAX;
    float G2 = (Y1.x + guv) * CV_8U_MAX;
    float B2 = (Y1.x + buv) * CV_8U_MAX;

    float R3 = (Y2.x + ruv) * CV_8U_MAX;
    float G3 = (Y2.x + guv) * CV_8U_MAX;
    float B3 = (Y2.x + buv) * CV_8U_MAX;

    float R4 = (Y3.x + ruv) * CV_8U_MAX;
    float G4 = (Y3.x + guv) * CV_8U_MAX;
    float B4 = (Y3.x + buv) * CV_8U_MAX;

    int row_step = dst_w * 3;
    __global uchar* pDstRow1 = pBGR + (dst_y * row_step + 3 * dst_x);
    __global uchar* pDstRow2 = pDstRow1 + row_step;

    pDstRow1[0] = convert_uchar_sat(B1);
    pDstRow1[1] = convert_uchar_sat(G1);
    pDstRow1[2] = convert_uchar_sat(R1);
    pDstRow1[3] = convert_uchar_sat(B2);
    pDstRow1[4] = convert_uchar_sat(G2);
    pDstRow1[5] = convert_uchar_sat(R2);

    pDstRow2[0] = convert_uchar_sat(B3);
    pDstRow2[1] = convert_uchar_sat(G3);
    pDstRow2[2] = convert_uchar_sat(R3);
    pDstRow2[3] = convert_uchar_sat(B4);
    pDstRow2[4] = convert_uchar_sat(G4);
    pDstRow2[5] = convert_uchar_sat(R4);
}


__kernel
void crop_resize_csc_gray(
                __read_only image2d_t img_y_src,
                __read_only image2d_t img_uv_src,
                uint src_w, uint src_h,
                uint crop_x, uint crop_y,
                uint crop_w, uint crop_h,
                __global unsigned char* pBGR,
                uint dst_w, uint dst_h)
{
    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int dst_x = 2 * id_x;
    int dst_y = 2 * id_y;

    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                        CLK_ADDRESS_CLAMP_TO_EDGE   |
                        CLK_FILTER_LINEAR;

    int dst_x0 = dst_x;
    int dst_x1 = dst_x + 1;
    int dst_y0 = dst_y;
    int dst_y1 = dst_y + 1;

    int x0 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y0 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x1 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y1 = crop_y + (dst_y0 * crop_h + dst_h/2)/dst_h;
    int x2 = crop_x + (dst_x0 * crop_w + dst_w/2)/dst_w;
    int y2 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;
    int x3 = crop_x + (dst_x1 * crop_w + dst_w/2)/dst_w;
    int y3 = crop_y + (dst_y1 * crop_h + dst_h/2)/dst_h;

    float4  Y0 = read_imagef (img_y_src, sampler, (int2)(x0, y0));
    float4  Y1 = read_imagef (img_y_src, sampler, (int2)(x1, y1));
    float4  Y2 = read_imagef (img_y_src, sampler, (int2)(x2, y2));
    float4  Y3 = read_imagef (img_y_src, sampler, (int2)(x3, y3));

    __global uchar* pDstRow1 = pBGR + (dst_y * dst_w + dst_x);
    __global uchar* pDstRow2 = pDstRow1 + dst_w;

    pDstRow1[0] = convert_uchar_sat(Y0.x * CV_8U_MAX);
    pDstRow1[1] = convert_uchar_sat(Y1.x * CV_8U_MAX);
    pDstRow2[0] = convert_uchar_sat(Y2.x * CV_8U_MAX);
    pDstRow2[1] = convert_uchar_sat(Y3.x * CV_8U_MAX);
}
