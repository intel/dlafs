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

#ifndef __GOOGLENETV2_ALGO_H__
#define __GOOGLENETV2_ALGO_H__

#include "algobase.h"

class GoogleNetv2Algo : public CvdlAlgoBase 
{
public:
    GoogleNetv2Algo();
    virtual ~GoogleNetv2Algo();

    //virtual void set_data_caps(GstCaps *incaps);
    //virtual GstBuffer* dequeue_buffer();
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                      int precision, CvdlAlgoData *outData, int objId);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);

    guint64 mCurPts;
};

#endif
