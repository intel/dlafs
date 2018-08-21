/*
 * Copyright (c) 2017-2018 Intel Corporation
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


__kernel void blend( __read_only image2d_t src_y,
                     __read_only image2d_t src_uv,
                     __read_only image2d_t src_osd,
                    __write_only image2d_t dst_rgb,
                     int dst_w, int dst_h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int id_z = 2 * id_x;
    int id_w = 2 * id_y;

    float4 Y0, Y1, Y2, Y3, UV;
    float4 rgba[4];

    Y0 = read_imagef(src_y, sampler, (int2)(id_z    , id_w));
    Y1 = read_imagef(src_y, sampler, (int2)(id_z + 1, id_w));
    Y2 = read_imagef(src_y, sampler, (int2)(id_z    , id_w + 1));
    Y3 = read_imagef(src_y, sampler, (int2)(id_z + 1, id_w + 1));
    UV = read_imagef(src_uv,sampler,(int2)(id_z/2  , id_w / 2)) - d2;

    rgba[0] = read_imagef(src_osd, sampler, (int2)(id_z   , id_w));
    rgba[1] = read_imagef(src_osd, sampler, (int2)(id_z +1, id_w));
    rgba[2] = read_imagef(src_osd, sampler, (int2)(id_z   , id_w + 1));
    rgba[3] = read_imagef(src_osd, sampler, (int2)(id_z +1, id_w + 1));

    __constant float* coeffs = c_YUV2RGBCoeffs_420;

    Y0 = max(0.f, Y0 - d1) * coeffs[0];
    Y1 = max(0.f, Y1 - d1) * coeffs[0];
    Y2 = max(0.f, Y2 - d1) * coeffs[0];
    Y3 = max(0.f, Y3 - d1) * coeffs[0];

    float ruv = fma(coeffs[4], UV.y, 0.0f);
    float guv = fma(coeffs[3], UV.y, fma(coeffs[2], UV.x, 0.0f));
    float buv = fma(coeffs[1], UV.x, 0.0f);

    float R0 = (Y0.x + ruv);
    float G0 = (Y0.x + guv);
    float B0 = (Y0.x + buv);

    float R1 = (Y1.x + ruv);
    float G1 = (Y1.x + guv);
    float B1 = (Y1.x + buv);

    float R2 = (Y2.x + ruv);
    float G2 = (Y2.x + guv);
    float B2 = (Y2.x + buv);

    float R3 = (Y3.x + ruv);
    float G3 = (Y3.x + guv);
    float B3 = (Y3.x + buv);

	float4 p0, p1, p2, p3;
	p0 = (float4)(R0, G0, B0, 0.5) + rgba[0];
    p1 = (float4)(R1, G1, B1, 0.5) + rgba[1];
    p2 = (float4)(R2, G2, B2, 0.5) + rgba[2];
    p3 = (float4)(R3, G3, B3, 0.5) + rgba[3];

	//p0 = rgba[0];
	//p1 = rgba[1];//(float4) (0.5, 0.3, 0.2, 0.5);
	//p2 = rgba[2];//(float4) (0.5, 0.3, 0.2, 0.5);
	//p3 = rgba[3];//(float4) (0.5, 0.3, 0.2, 0.5);
    write_imagef(dst_rgb, (int2)(id_z    , id_w    ), p0);
    write_imagef(dst_rgb, (int2)(id_z + 1, id_w    ), p1);
    write_imagef(dst_rgb, (int2)(id_z    , id_w + 1), p2);
    write_imagef(dst_rgb, (int2)(id_z + 1, id_w + 1), p3);
}

#if 0
static const __constant float CV_8U_MAX         = 255.0f;
static const __constant float CV_8U_SCALE       = 1.0f / 255.0f;
__kernel void blend(__global unsigned char* dst_y,
                    __global unsigned char* dst_uv,
                    __read_only image2d_t fg,
                    __global unsigned char* pDstY,
                    __global unsigned char* pDstUV,
                    int dst_w, int dst_h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int id_z = 2 * id_x;
    int id_w = 2 * id_y;

    float4 y1, y2, y3, y4;
    float4 y1_dst, y2_dst, y3_dst, y4_dst, y_dst;
    float4 uv, uv_dst;
    float4 rgba[4];

    __global uchar* pDstRow1Y  = dst_y     + (id_w * dst_w + id_z);
    __global uchar* pDstRow2Y  = pDstRow1Y + dst_w;
    __global uchar* pDstRowUV  = dst_uv    + (id_w/2) * (dst_w/2) + id_z/2;

	y1 = (float4)(pDstRow1Y[0]*CV_8U_SCALE, 0.0, 0.0, 0.0);
	y2 = (float4)(pDstRow1Y[1]*CV_8U_SCALE, 0.0, 0.0, 0.0);
	y3 = (float4)(pDstRow2Y[0]*CV_8U_SCALE, 0.0, 0.0, 0.0);
	y4 = (float4)(pDstRow2Y[1]*CV_8U_SCALE, 0.0, 0.0, 0.0);
	uv = (float4)(pDstRowUV[0]*CV_8U_SCALE, pDstRowUV[1]*CV_8U_SCALE, 0.0, 0.0);

    rgba[0] = read_imagef(fg, sampler, (int2)(id_z   , id_w));
    rgba[1] = read_imagef(fg, sampler, (int2)(id_z +1, id_w));
    rgba[2] = read_imagef(fg, sampler, (int2)(id_z   , id_w + 1));
    rgba[3] = read_imagef(fg, sampler, (int2)(id_z +1, id_w + 1));

    y_dst = 0.299f * (float4) (rgba[0].x, rgba[1].x, rgba[2].x, rgba[3].x);
    y_dst = mad(0.587f, (float4) (rgba[0].y, rgba[1].y, rgba[2].y, rgba[3].y), y_dst);
    y_dst = mad(0.114f, (float4) (rgba[0].z, rgba[1].z, rgba[2].z, rgba[3].z), y_dst);
    y_dst *= (float4) (rgba[0].w, rgba[1].w, rgba[2].w, rgba[3].w);
    y1_dst = mad(1 - rgba[0].w, y1, y_dst.x);
    y2_dst = mad(1 - rgba[1].w, y2, y_dst.y);
    y3_dst = mad(1 - rgba[2].w, y3, y_dst.z);
    y4_dst = mad(1 - rgba[3].w, y4, y_dst.w);

    uv_dst.x = rgba[0].w * (-0.14713f * rgba[0].x - 0.28886f * rgba[0].y + 0.43600f * rgba[0].z + 0.5f);
    uv_dst.y = rgba[0].w * ( 0.61500f * rgba[0].x - 0.51499f * rgba[0].y - 0.10001f * rgba[0].z + 0.5f);
    uv_dst.x = mad(1 - rgba[0].w, uv.x, uv_dst.x);
    uv_dst.y = mad(1 - rgba[0].w, uv.y, uv_dst.y);

	pDstRow1Y[0] = convert_uchar_sat(y1_dst.x);
	pDstRow1Y[1] = convert_uchar_sat(y2_dst.x);
	pDstRow2Y[0] = convert_uchar_sat(y3_dst.x);
	pDstRow2Y[1] = convert_uchar_sat(y4_dst.x);
	pDstRowUV[0] = convert_uchar_sat(uv_dst.x);
	pDstRowUV[1] = convert_uchar_sat(uv_dst.y);
}
#endif


 #if 0
__kernel void blend( __read_write image2d_t dst_y,
                     __read_write image2d_t dst_uv,
                    __read_only image2d_t fg)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int id_z = 2 * id_x;
    int id_w = 2 * id_y;

    float4 y1, y2, y3, y4;
    float4 y1_dst, y2_dst, y3_dst, y4_dst, y_dst;
    float4 uv, uv_dst;
    float4 rgba[4];

    y1 = read_imagef(dst_y, sampler, (int2)(id_z    , id_w));
    y2 = read_imagef(dst_y, sampler, (int2)(id_z + 1, id_w));
    y3 = read_imagef(dst_y, sampler, (int2)(id_z    , id_w + 1));
    y4 = read_imagef(dst_y, sampler, (int2)(id_z + 1, id_w + 1));
    uv = read_imagef(dst_uv, sampler,(int2)(id_z/2  , id_w / 2));

    rgba[0] = read_imagef(fg, sampler, (int2)(id_z   , id_w));
    rgba[1] = read_imagef(fg, sampler, (int2)(id_z +1, id_w));
    rgba[2] = read_imagef(fg, sampler, (int2)(id_z   , id_w + 1));
    rgba[3] = read_imagef(fg, sampler, (int2)(id_z +1, id_w + 1));

    y_dst = 0.299f * (float4) (rgba[0].x, rgba[1].x, rgba[2].x, rgba[3].x);
    y_dst = mad(0.587f, (float4) (rgba[0].y, rgba[1].y, rgba[2].y, rgba[3].y), y_dst);
    y_dst = mad(0.114f, (float4) (rgba[0].z, rgba[1].z, rgba[2].z, rgba[3].z), y_dst);
    y_dst *= (float4) (rgba[0].w, rgba[1].w, rgba[2].w, rgba[3].w);
    y1_dst = mad(1 - rgba[0].w, y1, y_dst.x);
    y2_dst = mad(1 - rgba[1].w, y2, y_dst.y);
    y3_dst = mad(1 - rgba[2].w, y3, y_dst.z);
    y4_dst = mad(1 - rgba[3].w, y4, y_dst.w);

    uv_dst.x = rgba[0].w * (-0.14713f * rgba[0].x - 0.28886f * rgba[0].y + 0.43600f * rgba[0].z + 0.5f);
    uv_dst.y = rgba[0].w * ( 0.61500f * rgba[0].x - 0.51499f * rgba[0].y - 0.10001f * rgba[0].z + 0.5f);
    uv_dst.x = mad(1 - rgba[0].w, uv.x, uv_dst.x);
    uv_dst.y = mad(1 - rgba[0].w, uv.y, uv_dst.y);

    // Don't support write_read mode?
    write_imagef(dst_y, (int2)(id_z    , id_w    ), y1_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w    ), y2_dst);
    write_imagef(dst_y, (int2)(id_z    , id_w + 1), y3_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w + 1), y4_dst);
    write_imagef(dst_uv,(int2)(id_z / 2, id_w / 2), uv_dst);
}
#endif

#if 0
__kernel void blend(__read_only image2d_t dst_y,
                    __read_only image2d_t dst_uv,
                    __read_only image2d_t fg,
                    __global unsigned char* pDstY,
                    __global unsigned char* pDstUV,
                    int dst_w, int dst_h)
{
    sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    int id_x = get_global_id(0);
    int id_y = get_global_id(1);
    int id_z = 2 * id_x;
    int id_w = 2 * id_y;

    float4 y1, y2, y3, y4;
    float4 y1_dst, y2_dst, y3_dst, y4_dst, y_dst;
    float4 uv, uv_dst;
    float4 rgba[4];

    y1 = read_imagef(dst_y, sampler, (int2)(id_z    , id_w));
    y2 = read_imagef(dst_y, sampler, (int2)(id_z + 1, id_w));
    y3 = read_imagef(dst_y, sampler, (int2)(id_z    , id_w + 1));
    y4 = read_imagef(dst_y, sampler, (int2)(id_z + 1, id_w + 1));
    uv = read_imagef(dst_uv, sampler,(int2)(id_z/2  , id_w / 2));

    rgba[0] = read_imagef(fg, sampler, (int2)(id_z   , id_w));
    rgba[1] = read_imagef(fg, sampler, (int2)(id_z +1, id_w));
    rgba[2] = read_imagef(fg, sampler, (int2)(id_z   , id_w + 1));
    rgba[3] = read_imagef(fg, sampler, (int2)(id_z +1, id_w + 1));

    y_dst = 0.299f * (float4) (rgba[0].x, rgba[1].x, rgba[2].x, rgba[3].x);
    y_dst = mad(0.587f, (float4) (rgba[0].y, rgba[1].y, rgba[2].y, rgba[3].y), y_dst);
    y_dst = mad(0.114f, (float4) (rgba[0].z, rgba[1].z, rgba[2].z, rgba[3].z), y_dst);
    y_dst *= (float4) (rgba[0].w, rgba[1].w, rgba[2].w, rgba[3].w);
    y1_dst = mad(1 - rgba[0].w, y1, y_dst.x);
    y2_dst = mad(1 - rgba[1].w, y2, y_dst.y);
    y3_dst = mad(1 - rgba[2].w, y3, y_dst.z);
    y4_dst = mad(1 - rgba[3].w, y4, y_dst.w);

    uv_dst.x = rgba[0].w * (-0.14713f * rgba[0].x - 0.28886f * rgba[0].y + 0.43600f * rgba[0].z + 0.5f);
    uv_dst.y = rgba[0].w * ( 0.61500f * rgba[0].x - 0.51499f * rgba[0].y - 0.10001f * rgba[0].z + 0.5f);
    uv_dst.x = mad(1 - rgba[0].w, uv.x, uv_dst.x);
    uv_dst.y = mad(1 - rgba[0].w, uv.y, uv_dst.y);

#if 0
    // Don't support write_read mode?
    write_imagef(dst_y, (int2)(id_z    , id_w    ), y1_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w    ), y2_dst);
    write_imagef(dst_y, (int2)(id_z    , id_w + 1), y3_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w + 1), y4_dst);
    write_imagef(dst_uv,(int2)(id_z / 2, id_w / 2), uv_dst);
#else
    __global uchar* pDstRow1Y  = pDstY  + (id_w    * dst_w     + id_z);
    __global uchar* pDstRow2Y  = pDstRow1Y + dst_w;
    __global uchar* pDstRowUV  = pDstUV + (id_w/2) * (dst_w/2) + id_z/2;

	pDstRow1Y[0] = convert_uchar_sat(y1_dst.x);
	pDstRow1Y[1] = convert_uchar_sat(y2_dst.x);
	pDstRow2Y[0] = convert_uchar_sat(y3_dst.x);
	pDstRow2Y[1] = convert_uchar_sat(y4_dst.x);
	pDstRowUV[0] = convert_uchar_sat(uv_dst.x);
	pDstRowUV[1] = convert_uchar_sat(uv_dst.y);
#endif
}
#endif
