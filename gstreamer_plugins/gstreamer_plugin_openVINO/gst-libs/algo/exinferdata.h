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

#ifndef __EX_INFERENCE_DATA_H__
#define __EX_INFERENCE_DATA_H__

#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ExDataType{
    DataTypeBool = 0,
    DataTypeInt8,
    DataTypeUint8,
    DataTypeInt16,
    DataTypeUint16,
    DataTypeInt32,
    DataTypeUint32,
    DataTypeFP16,
    DataTypeFP32
}ExDataType;

typedef struct _ExObjectData {
    int id;
    int objectClass;
    float prob;
    std::string  label;
    // It is based on the orignal video frame
    int x;
    int y;
    int w;
    int h;
}ExObjectData;

typedef struct _ExInferData {
//public:
//    ExInferData();
//    ~ExInferData() {mObjectVec.clear();}
    std::vector<ExObjectData> mObjectVec;
    // output index, for multiple output algo, default is 0
    int outputIndex;
}ExInferData;

typedef ExInferData * (*pfInferenceResultParseFunc)(void *data,  ExDataType type, int len,int image_width, int image_height); 
typedef int  (*pfPostProcessInferenceDataFunc)(ExInferData &data);
typedef void (*pfGetDataTypeFunc)(ExDataType *inData,  ExDataType *outData);
typedef void (*pfGetMeanScaleFunc)(float *mean, float *scale);
typedef char *(*pfGetNetworkConfigFunc)(const char* modelName);

#ifdef __cplusplus
};
#endif
#endif