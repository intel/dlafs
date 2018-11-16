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
#include "samplealgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace HDDLStreamFilter;
using namespace std;


static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
}

SampleAlgo::SampleAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    set_default_label_name();

    mInCaps = NULL;
}

SampleAlgo::~SampleAlgo()
{
    g_print("SampleAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

void SampleAlgo::set_default_label_name()
{

}

void SampleAlgo::set_label_names(const char** label_names)
{
    mLabelNames = label_names;
}

void SampleAlgo::set_data_caps(GstCaps *incaps)
{
    // load IE and cnn model
    std::string filenameXML;
    const gchar *env = g_getenv("CVDL_Sample_MODEL_FULL_PATH");
    if(env){
        filenameXML = std::string(env);
    }else{
        filenameXML = std::string(CVDL_MODEL_DIR_DEFAULT"/Sample_detect/MobileNetSample.xml");
    }
    algo_dl_init(filenameXML.c_str());
    init_dl_caps(incaps);
}

GstFlowReturn SampleAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    // set in/out precision
    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(127.0f,  1.0/127.0f);
    ret = init_ieloader(modeFileName, IE_MODEL_LP_NONE);

    return ret;
}



GstFlowReturn SampleAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("SampleAlgo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    //parse inference result and put them into algoData
    g_print("input data = %p\n", input);

    return GST_FLOW_OK;
}
    

