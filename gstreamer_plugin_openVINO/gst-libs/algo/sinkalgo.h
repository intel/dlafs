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

#ifndef __SINK_ALGO_H__
#define __SINK_ALGO_H__

#include <vector>
#include "algobase.h"
#include <gst/gstbuffer.h>
#include "algopipeline.h"

class SinkAlgo : public CvdlAlgoBase 
{
public:
    SinkAlgo();
    virtual ~SinkAlgo();
    virtual GstBuffer* dequeue_buffer();
    virtual void set_data_caps(GstCaps *incaps)
    {
        //DO nothing
    }

   void set_linked_item(CvdlAlgoBase *item, int index)
   {
        if(index>=0 && index<MAX_PRE_SINK_ALGO_NUM)
            pLinkedItem[index] = item;
        else
            g_print("Failed to set linked item to sinkAlgo!\n");
   }
private:
    CvdlAlgoBase *pLinkedItem[MAX_PRE_SINK_ALGO_NUM];
};

#endif
