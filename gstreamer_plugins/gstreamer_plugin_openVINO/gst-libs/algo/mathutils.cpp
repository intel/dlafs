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

#include "mathutils.h"

float MathUtils::overlap(float x1, float w1, float x2, float w2)
{
    float t1, t2, t3, t4;

    float halfW1 = w1 / 2.0f;
    float halfW2 = w2 / 2.0f;

    t1 = x1 - halfW1;
    t2 = x2 - halfW2;
    t3 = x1 + halfW1;
    t4 = x2 + halfW2;

    return MIN(t3, t4) - MAX(t1, t2);
}

float MathUtils::overlap_rect(RectF a, RectF b)
{
    float width = overlap(a.x, a.w, b.x, b.w);
    float height = overlap(a.y, a.h, b.y, b.h);

    if (width < 0.0 || height < 0.0) {
        return 0.0;
    }

    return width * height;
}

float MathUtils::box_union(RectF a, RectF b)
{
    float i = overlap_rect(a, b);
    float u = a.w * a.h + b.w * b.h - i;
    return u;
}

float MathUtils::box_iou(RectF a, RectF b)
{
    float union_area = box_union(a, b);
    if(union_area <= 0.0)
        return 0.0;

    return overlap_rect(a, b) / box_union(a, b);
}

int MathUtils::get_max_index(float *ptr, int n)
{
    if (n <= 0)
        return -1;
    int i, index = 0;
    float max = ptr[0];
    for (i = 1; i < n; ++i) {
        if (ptr[i] > max) {
            max = ptr[i];
            index = i;
        }
    }
    return index;
}

cv::Rect MathUtils::verify_rect(int W, int H, cv::Rect rt)
{
    cv::Rect rtRet;

    rtRet.x = MAX(rt.x, 0);
    rtRet.y = MAX(rt.y, 0);

    int x2 = rt.x + rt.width;
    int y2 = rt.y + rt.height;

    x2 = MIN(x2, W);
    y2 = MIN(y2, H);

    rtRet.width = x2 - rtRet.x;
    rtRet.height = y2 - rtRet.y;

    return rtRet;
}

/**
 * @brief Calc two rectangles overlap ratio
 */
void MathUtils::get_overlap_ratio(cv::Rect rt1,
    cv::Rect rt2, float& ratio12, float& ratio21)
{
    int x1 = MAX(rt1.x, rt2.x);
    int y1 = MAX(rt1.y, rt2.y);
    int x2 = MIN(rt1.x + rt1.width, rt2.x + rt2.width);
    int y2 = MIN(rt1.y + rt1.height, rt2.y + rt2.height);

    if (x2 < x1 || y2 < y1) {
        ratio12 = ratio21 = 0;
        return;
    }

    float ol = (float) ((y2 - y1) * (x2 - x1));

    ratio12 = ol / (rt2.width * rt2.height);
    ratio21 = ol / (rt1.width * rt1.height);
}

cv::Rect MathUtils::convert_rect(cv::Rect rect,
    int oldBaseW, int oldBaseH, int newBaseW, int newBaseH)
{
    cv::Rect rt;

    rt.x     = rect.x     * newBaseW / oldBaseW;
    rt.width = rect.width * newBaseW / oldBaseW;
    rt.y     = rect.y     * newBaseH / oldBaseH;
    rt.height= rect.height* newBaseH / oldBaseH;

    return rt;
}

double MathUtils::dotProduct(float * vec1, float * vec2, int length)
{
    double sum = 0;
    for(int i = 0; i < length; i ++){
        sum += (double)vec1[i] * (double)vec2[i];
    }
    return sum;
}

float  MathUtils::norm(float * vec, int length)
{
    double squreSum = dotProduct(vec, vec, length);
    return sqrt(squreSum);
}

float  MathUtils::cosDistance(float * vec1, float * vec2, int len)
{
    double norm1 = norm(vec1, len);
    double norm2 = norm(vec2, len);
    double dotProd = dotProduct(vec1, vec2, len);

    if(norm1 <= 0.00001 || norm2 <= 0.00001){
        return 0;
    }

    double distance = dotProd / (norm1 * norm2);
    return (float)distance;
}

