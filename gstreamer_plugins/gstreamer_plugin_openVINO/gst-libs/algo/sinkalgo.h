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
