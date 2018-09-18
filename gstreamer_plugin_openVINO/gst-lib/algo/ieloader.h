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

#ifndef __IE_LOADER_H__
#define __IE_LOADER_H__

#include <string>
#include <vector>
#include <map>
#include <vector>
#include <tuple>
#include <mutex>
#include <condition_variable>

#include <opencv2/opencv.hpp>
#include <inference_engine.hpp>

#include "algobase.h"

// model directory, alt it can be passed from app
#define MODEL_DIR "/usr/lib/x86_64-linux-gnu/libgstcvdl/models"

#define CHECK(X) if(!(X)){ GST_ERROR("CHECK ERROR!"); std::exit(EXIT_FAILURE); }
#define REQUEST_NUM 8


enum{
    IE_MODEL_DETECTION = 0,
    IE_MODEL_CLASSFICATION = 1,
};


class IELoader {
public:
    IELoader();
    ~IELoader();

    GstFlowReturn set_device(InferenceEngine::TargetDevice dev);
    GstFlowReturn read_model(std::string strModelXml, std::string strModelBin, int modelType);
    GstFlowReturn convert_input_to_blob(const cv::UMat& img, InferenceEngine::Blob::Ptr& inputBlobPtr);

    // move to algoBase
    // parse inference result for a frame, which may contain mutiple objects
    //virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr, int precision,
    //                                                    CvdlAlgoData *outData, int objId);
    GstFlowReturn do_inference_async(CvdlAlgoData *algoData, uint64_t frmId, int objId,
                                            cv::UMat &src, AsyncCallback cb);
    int get_enable_request();
    GstFlowReturn get_input_size(int *w, int *h, int *c);

    InferenceEngine::TargetDevice mTargetDev;
	InferenceEngine::Precision mInputPrecision = InferenceEngine::Precision::U8;
	InferenceEngine::Precision mOutputPrecision = InferenceEngine::Precision::FP32;

    std::string mModelXml;
    std::string mModelBin;

    InferenceEngine::InferenceEnginePluginPtr mIEPlugin; // plugin must be first so it would be last in the destruction order
    InferenceEngine::IExecutableNetwork::Ptr  mExeNetwork;

    std::string mFirstInputName;
    std::string mFirstOutputName;

    std::mutex mRequstMutex;
    std::condition_variable mCondVar;
   
    // <request, used or not>
    InferenceEngine::IInferRequest::Ptr mInferRequest[REQUEST_NUM];
    bool mRequestEnable[REQUEST_NUM] = {true};
};

#endif
