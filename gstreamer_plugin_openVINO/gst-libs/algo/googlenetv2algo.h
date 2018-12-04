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

#ifndef __GOOGLENETV2_ALGO_H__
#define __GOOGLENETV2_ALGO_H__

#include "algobase.h"

class GoogleNetv2Algo : public CvdlAlgoBase 
{
public:
    GoogleNetv2Algo();
    virtual ~GoogleNetv2Algo();

    virtual void set_data_caps(GstCaps *incaps);
    //virtual GstBuffer* dequeue_buffer();
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                      int precision, CvdlAlgoData *outData, int objId);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);

    guint64 mCurPts;
};

#endif
