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


#ifndef __HDDLS_ALGO_PRIVATE_H__
#define  __HDDLS_ALGO_PRIVATE_H__

#define QUEUE_ELEMENT_MAX_NUM 100

// log dump to this directory 
#define LOG_DIR "~/hddls_log/"

// Each algo in the algo chain can link to multiple downstream algo
// Here we set 2 by default
#define MAX_DOWN_STREAM_ALGO_NUM 2

// PIPELINE can output multiple output,  one output with one buffer_queue
// Here we set 1
#define MAX_PIPELINE_OUT_NUM 1

// SinkAlgo item can accept multiple preItem, one is a algo branch
#define MAX_PRE_SINK_ALGO_NUM 2


const static guint CVDL_OBJECT_FLAG_DONE =0x1;

const static guint  CVDL_TYPE_DL = 0;
const static guint  CVDL_TYPE_CV = 1;
const static guint  CVDL_TYPE_NONE = 2;

#endif
