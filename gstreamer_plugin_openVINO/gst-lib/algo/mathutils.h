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

#ifndef __MATH_UTILS_H__
#define __MATH_UTILS_H__

#include <opencv2/opencv.hpp>

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif
#ifndef MAX
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif

#ifndef FLT2INT
#define FLT2INT(X) (int)((X) + 0.5f)
#endif

typedef struct {
	float x;
	float y;
	float w;
	float h;
}RectF;

class MathUtils{
public:
    float overlap(float x1, float w1, float x2, float w2);
    float overlap_rect(RectF a, RectF b);
    float box_union(RectF a, RectF b);
    float box_iou(RectF a, RectF b);

    int get_max_index(float *ptr, int n);
    cv::Rect verify_rect(int W, int H, cv::Rect rt);
    void get_overlap_ratio(cv::Rect rt1, cv::Rect rt2, float& ratio12, float& ratio21);
    cv::Rect convert_rect(cv::Rect rect, int oldBaseW, int oldBaseH, int newBaseW, int newBaseH);
};

#endif
