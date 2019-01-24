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


#ifndef __EX_INFERENCE_PARSER_H__
#define __EX_INFERENCE_PARSER_H__
#include "exinferdata.h"

// Must free the pointer returned

// parser the inference data for one object
ExInferData* parse_inference_result(void *in_data, int data_type, int data_len, int image_width, int image_height);

// post process all objects for this frame
// return 1  - something has been changed
// return 0  - nothing has been changed
int  post_process_inference_data(ExInferData *data);

// get the data type for inference input and result
void get_data_type(ExDataType *inData,  ExDataType *outData);
void get_mean_scale(float *mean, float *scale);
// get network config for VPU_CONFIG
char *get_network_config(const char* modelName);
#endif
