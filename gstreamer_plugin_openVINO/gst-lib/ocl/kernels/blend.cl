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
__kernel void blend(image2d_t dst_y,
                    image2d_t dst_uv,
                    __read_only  image2d_t fg)
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
    float4 temp;

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


    write_imagef(dst_y, (int2)(id_z    , id_w    ), y1_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w    ), y2_dst);
    write_imagef(dst_y, (int2)(id_z    , id_w + 1), y3_dst);
    write_imagef(dst_y, (int2)(id_z + 1, id_w + 1), y4_dst);
    write_imagef(dst_uv,(int2)(id_z / 2, id_w / 2), uv_dst);
}
