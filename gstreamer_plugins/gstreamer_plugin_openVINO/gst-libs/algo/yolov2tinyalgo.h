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
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

#ifndef __YOLO_V2_TINY_ALGO_H__
#define __YOLO_V2_TINY_ALGO_H__

#include "algobase.h"
#include "ieloader.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include "mathutils.h"

class Yolov2TinyAlgo : public CvdlAlgoBase 
{
public:
    Yolov2TinyAlgo();
    virtual ~Yolov2TinyAlgo();
    //virtual void set_data_caps(GstCaps *incaps);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId);
    void set_label_names(const char** label_names) { mLabelNames = label_names; }

private:
    void set_default_label_name();
    bool parse (const float * output, CvdlAlgoData* &outData);
    guint64 mCurPts;
    static const int mGridWidth = 13;
    static const int mGridHeight = 13;
    const char** mLabelNames;
};

struct YoloBox
{
    int classId;
    float prob;
    float objectness;
    float x1;
    float y1;
    float x2;
    float y2;
};

#endif

