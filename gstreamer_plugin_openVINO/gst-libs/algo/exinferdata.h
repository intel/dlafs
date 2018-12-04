
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