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



#include <string>
#include "samplealgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>
#include <dlfcn.h>
#include "exinferdata.h"
#include "genericalgo.h"


using namespace HDDLStreamFilter;
using namespace std;


static void post_callback(CvdlAlgoData *algoData)
{
    // post process algoData
    GenericAlgo *genericAlgo = static_cast<GenericAlgo*> (algoData->algoBase);
    std::vector<ObjectData> &objDataVec = algoData->mObjectVec;
    ExInferData exInferData;
    int ret = 0;

    //convert objData to exInferData
    for(guint i=0;i<objDataVec.size();i++) {
        ExObjectData exObjData;
        genericAlgo->objdata_2_exobjdata(objDataVec[i], exObjData);
        exInferData.mObjectVec.push_back(exObjData);
    }

    // do post process
    if(genericAlgo->pfPostProcess)
        ret = genericAlgo->pfPostProcess(exInferData);

    // sync exInferData to objDataVec
    if(ret) {
        std::vector<ObjectData>::iterator it;
        for (it = objDataVec.begin(); it != objDataVec.end();) {
            ObjectData &objItem =(*it);
            guint index = -1;
            for(guint i=0;i<exInferData.mObjectVec.size();i++) {
                ExObjectData &exObjData = exInferData.mObjectVec[i];
                if(exObjData.id == (*it).id) {
                    index = i;
                    break;
                }
            }
            // not found, then remove this object
            if(index == (guint)(-1)) {
                it = objDataVec.erase(it);    //remove item.
                continue;
            }
            // update
            genericAlgo->exobjdata_2_objdata(exInferData.mObjectVec[index], objItem);
            ++it;
        }
        algoData->mOutputIndex = exInferData.outputIndex;
    }
}

GenericAlgo::GenericAlgo(const char *name) : CvdlAlgoBase(post_callback, CVDL_TYPE_DL),
        mHandler(NULL), pfParser(NULL), pfPostProcess(NULL), pfGetType(NULL),
        pfGetMS(NULL), pfGetNetworkConfig(NULL), mLoaded(false), inType(DataTypeInt8),
        outType(DataTypeFP32), mCurPts(0), mLabelNames(nullptr)
{
    mName = std::string(name);
    const gchar *env = g_getenv("HDDLS_CVDL_MODEL_PATH");
    GST_INFO("Create generic algo name = %s\n", name);
    if(env) {
         // $(CVDL_MODEL_FULL_PATH)/<algoname>/libalgo<algoname>.so
        mLibName = std::string(env) + std::string("/") + std::string(name) + std::string("/") 
                            + std::string("libalgo") + std::string(name) + std::string(".so");
        mHandler = dlopen(mLibName.c_str(), RTLD_LAZY);
        if(!mHandler)
            g_print("Failed to dlopen %s\n", mLibName.c_str());
    }else {
        g_print("Cannot get ${HDDLS_CVDL_MODEL_PATH}\n");
    }

    if(mHandler) {
          pfParser  = (pfInferenceResultParseFunc)dlsym(mHandler, "parse_inference_result");
          pfPostProcess  = (pfPostProcessInferenceDataFunc)dlsym(mHandler, "post_process_inference_data");
          pfGetType = (pfGetDataTypeFunc)dlsym(mHandler, "get_data_type");
          pfGetMS = (pfGetMeanScaleFunc)dlsym(mHandler, "get_mean_scale");
          pfGetNetworkConfig = (pfGetNetworkConfigFunc)dlsym(mHandler,"get_network_config");
    }

    if(pfParser && pfPostProcess &&  pfGetType && pfGetMS && pfGetNetworkConfig)
        mLoaded = true;
    else
        mLoaded = false;
}

GenericAlgo::~GenericAlgo()
{
    g_print("GenericAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
    if(mHandler)        
        dlclose(mHandler);
}

void GenericAlgo::set_default_label_name()
{
     //set default label name
}

GstFlowReturn GenericAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;
    ExDataType in, out;
    float mean=0.0, scale=1.0;
    std::string network_config=std::string("null");
    if(!mLoaded)
        return GST_FLOW_ERROR;

    // set in/out precision
    if(pfGetType) {
        pfGetType(&in, &out);
        mIeLoader.set_precision(getIEPrecision(in), getIEPrecision(out));
        inType = in;
        outType = out;
    }

    if(pfGetMS) {
         pfGetMS(&mean, &scale);
        mIeLoader.set_mean_and_scale(mean,  scale);
    }
    if(pfGetNetworkConfig) {
          char* config = pfGetNetworkConfig(modeFileName);
          if(config) {
                network_config  = std::string(config);
                free(config);
          }
    }
    ret = init_ieloader(modeFileName, IE_MODEL_GENERIC, network_config);

    return ret;
}

GstFlowReturn GenericAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("GenericAlgo::parse_inference_result begin: outData = %p\n", outData);
    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);
    //void *paserResult = NULL;

    if(!mLoaded)
        return GST_FLOW_ERROR;

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }
    int len =  resultBlobFp32->size();

    //parse inference result and put them into algoData
    GST_LOG("input data = %p\n", input);
    ExInferData *exInferData = NULL;
    if(pfParser) {
        exInferData = pfParser(input, outType, len,mImageProcessorInVideoWidth,mImageProcessorInVideoHeight);
        //exInferData = std::dynamic_pointer_cast<ExInferData>(paserResult);
        // copy exInferData to  algoData->mObjectVec;
        for(guint i=0; i<exInferData->mObjectVec.size();i++) {
            ObjectData objData;
            exobjdata_2_objdata(exInferData->mObjectVec[i], objData);
            outData->mObjectVec.push_back(objData);
        }
        outData->mOutputIndex = exInferData->outputIndex;
        delete exInferData;
    }
    return GST_FLOW_OK;
}
    
void GenericAlgo::exobjdata_2_objdata(ExObjectData &exObjData,  ObjectData &objData)
{
    objData.id = exObjData.id;
    objData.objectClass = exObjData.objectClass ;
    objData.prob = exObjData.prob;
    objData.label = exObjData.label;
    objData.rect.x = exObjData.x ;
    objData.rect.y = exObjData.y;
    objData.rect.width = exObjData.w;
    objData.rect.height = exObjData.h;
}

void GenericAlgo::objdata_2_exobjdata(ObjectData &objData,  ExObjectData &exObjData)
{
    exObjData.id = objData.id;
    exObjData.objectClass = objData.objectClass;
    exObjData.prob= objData.prob;
    exObjData.label= objData.label;
    exObjData.x = objData.rect.x;
    exObjData.y = objData.rect.y;
    exObjData.w = objData.rect.width;
    exObjData.h = objData.rect.height;
}


InferenceEngine::Precision GenericAlgo::getIEPrecision(ExDataType type) 
{
    switch(type) {
    case DataTypeInt8:
        return InferenceEngine::Precision::I8;
        break;
    case DataTypeUint8:
        return InferenceEngine::Precision::U8;
        break;
    case DataTypeInt16:
        return InferenceEngine::Precision::I16;
        break;
    case DataTypeUint16:
        return InferenceEngine::Precision::U16;
        break;
    case DataTypeInt32:
        return InferenceEngine::Precision::I32;
        break;
    case DataTypeFP16:
        return InferenceEngine::Precision::FP16;
        break;
    case DataTypeFP32:
        return InferenceEngine::Precision::FP32;
        break;
    default:
        //return InferenceEngine::Precision::FP32;
        break;
    }
    return  InferenceEngine::Precision::FP32;
}
