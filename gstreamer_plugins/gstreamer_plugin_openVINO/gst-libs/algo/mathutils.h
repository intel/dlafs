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

    double dotProduct(float * vec1, float * vec2, int length);
    float  norm(float * vec, int length);
    float  cosDistance(float * vec1, float * vec2, int len);
};

#endif
