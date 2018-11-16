
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
#include <string>
#include "classificationalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace std;

static std::string g_vehicleLabel[] =
{
#include "label-en.txt"
};

#define THRESHOLD_PROB 0.2
#define CLASSIFICATION_INPUT_W 224
#define CLASSIFICATION_INPUT_H 224

static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
}

ClassificationAlgo::ClassificationAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mInputWidth = CLASSIFICATION_INPUT_W;
    mInputHeight = CLASSIFICATION_INPUT_H;
}

ClassificationAlgo::~ClassificationAlgo()
{
      g_print("ClassificationAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
            mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
           1000000.0*mFrameDoneNum/mInferCost);
}

void ClassificationAlgo::set_data_caps(GstCaps *incaps)
{
    std::string filenameXML;
    const gchar *env = g_getenv("CVDL_CLASSIFICATION_MODEL_FULL_PATH");
    if(env) {
        filenameXML = std::string(env);
    }else{
        filenameXML = std::string(CVDL_MODEL_DIR_DEFAULT"/vehicle_classify/carmodel_fine_tune_1062_bn_iter_370000.xml");
    }
    algo_dl_init(filenameXML.c_str());
    init_dl_caps(incaps);
}

GstFlowReturn ClassificationAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    mIeLoader.set_precision(InferenceEngine::Precision::U8, InferenceEngine::Precision::FP32);
    ret = init_ieloader(modeFileName, IE_MODEL_CLASSFICATION);
    return ret;
}

GstFlowReturn ClassificationAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                  int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("ClassificationAlgo::parse_inference_result begin: outData = %p\n", outData);

    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    auto resultBlobFp32 
        = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);
    //float *input = static_cast<float*>(resultBlobFp32->data());

    // Get top N results
    std::vector<unsigned> topIndexes;
    int topnum = 1;
    ObjectData objData = outData->mObjectVecIn[objId];
    InferenceEngine::TopResults(topnum, *resultBlobFp32, topIndexes);
    float *probBase = resultBlobFp32->data();
    if (topIndexes.size()>0) {
        for (size_t i = 0; i < topIndexes.size(); i++) {
            float prob = probBase[topIndexes[i]];
            if(prob < THRESHOLD_PROB)
                continue;
            std::string strLabel = g_vehicleLabel[topIndexes[i]];
            objData.prob = prob;
            objData.label = strLabel;
            objData.objectClass =  topIndexes[i];
            outData->mObjectVec.push_back(objData);
            GST_LOG("classification-%ld-%d-%ld: prob = %f, label = %s\n", 
                outData->mFrameId,objId,i,prob,  objData.label.c_str());
            break;
        }
    }
    return GST_FLOW_OK;
}
