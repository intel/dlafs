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
#include "lprnetalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>
#include "algoregister.h"

using namespace std;

const char * NAME_MAPPING[] = {
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "Anhui",
        "Beijing",
        "Chongqing",
        "Fujian",
        "Gansu",
        "Guangdong",
        "Guangxi",
        "Guizhou",
        "Hainan",
        "Hebei",
        "Heilongjiang",
        "Henan",
        "HongKong",
        "Hubei",
        "Hunan",
        "InnerMongolia",
        "Jiangsu",
        "Jiangxi",
        "Jilin",
        "Liaoning",
        "Macau",
        "Ningxia",
        "Qinghai",
        "Shaanxi",
        "Shandong",
        "Shanghai",
        "Shanxi",
        "Sichuan",
        "Tianjin",
        "Tibet",
        "Xinjiang",
        "Yunnan",
        "Zhejiang",
        "police",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z"};

#define THRESHOLD_PROB 0.2
#define LP_INPUT_W 94
#define LP_INPUT_H 24

static void post_callback(CvdlAlgoData *algoData)
{
    std::vector<ObjectData> objectVecCp = algoData->mObjectVec;
    ObjectData objItem;
    algoData->mObjectVec.clear();
    for(unsigned int i=0; i<objectVecCp.size();i++) {
         objItem =  objectVecCp[i];
        objItem.label=std::string("vehicle");
        objItem.prob = 1.0;
        algoData->mObjectVec.push_back(objItem);//car
    
        objItem =  objectVecCp[i];
        objItem.rect = objItem.rectROI;
        algoData->mObjectVec.push_back(objItem); //LP
    }
}

LPRNetAlgo::LPRNetAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mName = std::string(ALGO_LPRNET_NAME);
    mSecData[0] = 1.0;
    for(int i=1;i<LPR_COLS;i++)
        mSecData[i] = 1.0;
}

LPRNetAlgo::~LPRNetAlgo()
{
    g_print("LPRNetAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

GstFlowReturn LPRNetAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(0.0f,  1.0f);
    mIeLoader.set_second_input(true,  (void *)mSecData, LPR_COLS, InferenceEngine::Precision::FP32);
    ret = init_ieloader(modeFileName, IE_MODEL_LP_RECOGNIZE);
    return ret;
}

GstFlowReturn LPRNetAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlob,
                                                               int precision, CvdlAlgoData *outData, int objId) {
    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float>>(resultBlob);
    const float * out = resultBlobFp32->data();
    std::string result;
    float prob_sum = 100;
    GstFlowReturn ret = GST_FLOW_OK;

    for(int col = 0; col < LPR_COLS; col ++){
        int idx = (int)(out[col]);
        if(idx >= LPR_ROWS - 1 || idx < 0){
            continue;
        }

        std::string info = NAME_MAPPING[idx];
        if( idx >= 10 && idx <= 43){
            info = std::string("<") + info + std::string(">");
        }
        result += info;
    }

    GST_LOG("lprnet: prob = %f, label = %s\n", prob_sum, result.c_str());
#if 1
    std::size_t pos = result.find("<");      // position of "<" in str
    if(pos>0 && pos<16) {
          std::string substr1 = result.substr (pos);
          std::string substr2 = result.substr (0, pos);
          result = substr1 + substr2;
           GST_LOG("(correct)lprnet: prob = %f, label = %s\n", prob_sum, result.c_str());
     }
#endif
    ObjectData objData = outData->mObjectVecIn[objId];
    //if(prob_sum >= 90) {
             objData.prob = prob_sum/100.0;
             objData.label = result;
             objData.objectClass =  0;
             outData->mObjectVec.push_back(objData);
     //} else {
     //    result.clear();
     //    ret = GST_FLOW_ERROR;
     //}

    return ret;
}
