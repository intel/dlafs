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
void MathUtils::get_overlap_ratio(cv::Rect rt1, cv::Rect rt2, float& ratio12, float& ratio21) {
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

cv::Rect MathUtils::convert_rect(cv::Rect rect, int oldBaseW, int oldBaseH, int newBaseW, int newBaseH)
{
    cv::Rect rt;

    rt.x     = rect.x     * newBaseW / oldBaseW;
    rt.width = rect.width * newBaseW / oldBaseW;
    rt.y     = rect.y     * newBaseH / oldBaseH;
    rt.height= rect.height* newBaseH / oldBaseH;

    return rt;
}

