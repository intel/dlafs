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


#include <inference_engine.hpp>
#include <ie_plugin_dispatcher.hpp>
#include <ie_plugin_config.hpp>
#include <ie_iexecutable_network.hpp>
#include <vpu/vpu_plugin_config.hpp>

#include <gst/gstbuffer.h>
#include <gst/gstpad.h>
#include <gst/gstinfo.h>
#include "ieloader.h"
#include "algobase.h"


#ifdef __WIN32__
#define HDDL_PLUGIN "HDDLPlugin.dll"
#else
#define HDDL_PLUGIN "libHDDLPlugin.so"
#endif

#ifdef __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,4,20)
#define CV_SPLIT_NO_OPENCL
#endif
#endif

#define IECALLCHECK(call)                                       \
if (InferenceEngine::OK != (call)) {                            \
    std::cout << #call " failed: " << resp.msg << std::endl;    \
    return GST_FLOW_ERROR;                                    \
}

#define IECALLNORETCHECK(call)                                       \
if (InferenceEngine::OK != (call)) {                            \
    std::cout << #call " failed: " << resp.msg << std::endl;    \
}

using namespace InferenceEngine;
using namespace std;

std::mutex requestCreateMutex;

#include <emmintrin.h> //SSE2(include xmmintrin.h)
static void data_norm_SSE(float* pfOut, unsigned char* pcInput,  int data_len, float scale, float mean)
{
    int i, len;
    const __m128i zero_16_uchar = _mm_setzero_si128(); // set 128bit to be zero
    const __m128i zero_8_short = _mm_set1_epi16(0);// return 8 short ,  r0=_W, r1=_W, ... r7=_W
    __m128 reg128_scale = _mm_set1_ps(scale); // r0 = r1 = r2 = r3 = w
    __m128 reg128_mean = _mm_set1_ps(-1.0*mean); // r0 = r1 = r2 = r3 = w

    len = data_len & (~15);
    for (i = 0; i < len; i += 16) {
        const __m128i src_16_uchar = _mm_loadu_si128((__m128i*)(pcInput+i)); // load 128bits data, not need 16 bytes aligned
        const __m128i src_lower_8_uchar_unpack_8_short = _mm_unpacklo_epi8(src_16_uchar, zero_16_uchar);   // First 8 short values
        __m128i src_lo_4_int = _mm_unpacklo_epi16(src_lower_8_uchar_unpack_8_short, zero_8_short); //r0=_A0, r1=_B0, r2=_A1, r3=_B1, r4=_A2, r5=_B2, r6=_A3, r7=_B3
        __m128i src_hi_4_int = _mm_unpackhi_epi16(src_lower_8_uchar_unpack_8_short, zero_8_short);

        __m128 src_lo_4_float = _mm_cvtepi32_ps(src_lo_4_int); // r0=(float)_A0, r1=(float)_A1, r2=(float)_A2, r3=(float)_A3
        __m128 dst_4_float_0 = _mm_mul_ps(_mm_add_ps(src_lo_4_float, reg128_mean), reg128_scale);
        __m128 src_hi_4_float = _mm_cvtepi32_ps(src_hi_4_int);
        __m128 dst_4_float_1 = _mm_mul_ps(_mm_add_ps(src_hi_4_float, reg128_mean), reg128_scale);

        const __m128i src_higher_8_uchar_unpack_8_short = _mm_unpackhi_epi8(src_16_uchar, zero_16_uchar);  // Last 8 short values

        src_lo_4_int = _mm_unpacklo_epi16(src_higher_8_uchar_unpack_8_short, zero_8_short);
        src_hi_4_int = _mm_unpackhi_epi16(src_higher_8_uchar_unpack_8_short, zero_8_short);

        src_lo_4_float = _mm_cvtepi32_ps(src_lo_4_int);
        __m128 dst_4_float_2 = _mm_mul_ps(_mm_add_ps(src_lo_4_float, reg128_mean), reg128_scale);

        src_hi_4_float = _mm_cvtepi32_ps(src_hi_4_int);
        __m128 dst_4_float_3 = _mm_mul_ps(_mm_add_ps(src_hi_4_float, reg128_mean), reg128_scale);

        _mm_store_ps(pfOut + i, dst_4_float_0);
        _mm_store_ps(pfOut + i + 4, dst_4_float_1);
        _mm_store_ps(pfOut + i + 8, dst_4_float_2);
        _mm_store_ps(pfOut + i + 12, dst_4_float_3);
    }

    for (i = len; i < data_len; ++i) {
        pfOut[i] = ((float)pcInput[i]  -mean )* scale;
    }
}

#if 0
static void data_norm_C(float* pfOut, unsigned char* pcInput,  int data_len,  float scale, float mean)
 {
    int i;
    for (i = 0; i < data_len; ++i) {
        pfOut[i] = ((float)pcInput[i]  -mean )* scale;
    }
}
#endif
IELoader::IELoader()
{
    mNeedSecondInputData = false;
}

IELoader::~IELoader()
{
    //IE will be release automatically.
}


GstFlowReturn IELoader::set_device(InferenceEngine::TargetDevice dev)
{
    mTargetDev = dev;
    switch (dev) {
    case InferenceEngine::TargetDevice::eCPU:
        mIEPlugin = InferenceEnginePluginPtr("libMKLDNNPlugin.so");
        break;
    case InferenceEngine::TargetDevice::eGPU:
        mIEPlugin = InferenceEnginePluginPtr("libclDNNPlugin.so");
        break;
    case InferenceEngine::TargetDevice::eHDDL:
        mIEPlugin = InferenceEnginePluginPtr(HDDL_PLUGIN);
        break;
    default:
        g_print("Not support device [ %d ]\n", (int)dev);
        return GST_FLOW_ERROR;
        break;
    }
    if(mIEPlugin)
        return GST_FLOW_OK;
    else
        return GST_FLOW_ERROR;
}
GstFlowReturn IELoader::read_model(std::string strModelXml,
        std::string strModelBin, int modelType, std::string network_config)
{
    std::unique_lock<std::mutex> _lock(requestCreateMutex);
    std::string config_xml;

    InferenceEngine::ResponseDesc resp;
    InferenceEngine::StatusCode ret = InferenceEngine::StatusCode::OK;

    InferenceEngine::CNNNetReader netReader = InferenceEngine::CNNNetReader();
    netReader.ReadNetwork(strModelXml);
    if (!netReader.isParseSuccess()) {
        g_print("read model %s fail", strModelXml.c_str());
        return GST_FLOW_ERROR;
    }
    //g_print("Success to read %s\n", strModelXml.c_str());

    netReader.ReadWeights(strModelBin);
    if (!netReader.isParseSuccess()) {
        g_print("read model %s fail", strModelBin.c_str());
        return GST_FLOW_ERROR;
    }
    //g_print("Success to read %s\n", strModelBin.c_str());

    InferenceEngine::CNNNetwork cnnNetwork = netReader.getNetwork();
    InferenceEngine::InputsDataMap networkInputs;
    InferenceEngine::OutputsDataMap networkOutputs;

    // Input data precision
    networkInputs = cnnNetwork.getInputsInfo();
    g_return_val_if_fail(networkInputs.empty()==FALSE, GST_FLOW_ERROR);
    auto inputInfo = networkInputs.begin();
    g_return_val_if_fail(inputInfo != networkInputs.end(), GST_FLOW_ERROR);
    inputInfo->second->setInputPrecision(mInputPrecision);
    mFirstInputName = inputInfo->first;
    inputInfo->second->setLayout(Layout::NCHW);//HW: NCHW, SW: NHWC

    if(mNeedSecondInputData) {
        inputInfo++;
        g_return_val_if_fail(inputInfo != networkInputs.end(), GST_FLOW_ERROR);
        inputInfo->second->setInputPrecision(mInputPrecision);
        mSecondInputName = inputInfo->first;
        GST_INFO("IE blobs second input name is %s\n", mSecondInputName.c_str());

        //Improvement need: make it generic!
        auto secondInputDims = inputInfo->second->getDims();
        if( secondInputDims.size() != 2 || secondInputDims[0] != 1 || secondInputDims[1] != mSecDataSrcCount){
            g_print("Parsing netowrk error (wrong size of second input).\n");
            return GST_FLOW_ERROR;
        }
    }

    // Output data precision
    networkOutputs = cnnNetwork.getOutputsInfo();
    g_return_val_if_fail(!networkOutputs.empty(), GST_FLOW_ERROR);
    auto firstOutputInfo = networkOutputs.begin();
    g_return_val_if_fail(firstOutputInfo != networkOutputs.end(), GST_FLOW_ERROR);
    firstOutputInfo->second->precision = mOutputPrecision;
    mFirstOutputName = firstOutputInfo->first;

    InferenceEngine::OutputsDataMap outputInfo(cnnNetwork.getOutputsInfo());
    auto outputInfoIter = outputInfo.begin();
    if(outputInfoIter  != outputInfo.end()){
        InferenceEngine::SizeVector outputDims = outputInfoIter->second->dims;
         mOutputDim[1] = (int)outputDims[1]; 
         mOutputDim[0] = (int)outputDims[0];
    }

    std::map<std::string, std::string> networkConfig;
    networkConfig[InferenceEngine::PluginConfigParams::KEY_LOG_LEVEL]
        = InferenceEngine::PluginConfigParams::LOG_INFO;
    networkConfig[VPU_CONFIG_KEY(HW_STAGES_OPTIMIZATION)] = CONFIG_VALUE(YES);

    mModelType = modelType;
    switch(modelType) {
        case IE_MODEL_DETECTION:
            networkConfig[VPU_CONFIG_KEY(NETWORK_CONFIG)] = "data=data,scale=64";
            break;
       case IE_MODEL_SSD:
            // Get moblienet_ssd_config_xml file name based on strModelXml
            config_xml = strModelXml.substr(0, strModelXml.rfind(".")) + std::string(".conf.xml");
            networkConfig[VPU_CONFIG_KEY(NETWORK_CONFIG)] = "file=" + config_xml;
            break;
        case IE_MODEL_LP_RECOGNIZE:
            break;
        case IE_MODEL_YOLOTINYV2:
            networkConfig[VPU_CONFIG_KEY(NETWORK_CONFIG)] = "data=input,scale=128";
            break;
        case IE_MODEL_GENERIC:
            if(network_config.compare("null"))
                networkConfig[VPU_CONFIG_KEY(NETWORK_CONFIG)] = network_config.c_str();
            break;
        default:
            break;
   }

    // Executable Network for inference engine
    ret = mIEPlugin->LoadNetwork(mExeNetwork, cnnNetwork, networkConfig, &resp);
    if (InferenceEngine::StatusCode::OK != ret) {
        // GENERAL_ERROR = -1
        g_print("Failed to  LoadNetwork, ret_code = %d, models=%s\n", ret, strModelBin.c_str());
        return GST_FLOW_ERROR;
    }

    // First create 16 request for current thread.
    for (int r = 0; r < REQUEST_NUM; r++) {
        IECALLCHECK(mExeNetwork->CreateInferRequest(mInferRequest[r], &resp));
        mRequestEnable[r] = true;
    }
    return GST_FLOW_OK;
}

GstFlowReturn IELoader::convert_input_to_blob(const cv::UMat& img,
    InferenceEngine::Blob::Ptr& inputBlobPtr)
{
    if (inputBlobPtr->precision() != mInputPrecision) {
        GST_ERROR("loadImage error: blob must have only same precision");
        return GST_FLOW_ERROR;
    }

    cv::Mat src;
    int w = (int)inputBlobPtr->dims()[0];
    int h = (int)inputBlobPtr->dims()[1];
    if (img.cols != w || img.rows != h) {
        GST_ERROR("WARNNING: resize from %dx%d to %dx%d !\n", src.cols, src.rows, w, h);
        cv::resize(img, src, cv::Size(w, h));
    } else {
#ifdef USE_OPENCV_3_4_x
        src = img.getMat(0);
#else
        src = img.getMat(cv::ACCESS_RW);
#endif
    }
    g_return_val_if_fail(src.data, GST_FLOW_ERROR);

    auto numBlobChannels = inputBlobPtr->dims()[2];
    size_t numImageChannels = src.channels();
    if (numBlobChannels != numImageChannels && numBlobChannels != 1) {
        GST_ERROR("numBlobChannels != numImageChannels && numBlobChannels != 1");
        return GST_FLOW_ERROR;
    }

    if (InferenceEngine::Precision::U8 == mInputPrecision) {
        InferenceEngine::TBlob<unsigned char>::Ptr inputBlobDataPtr = 
            std::dynamic_pointer_cast<InferenceEngine::TBlob<unsigned char> >(inputBlobPtr);
        if (inputBlobDataPtr != nullptr) {
            unsigned char *inputDataPtr = inputBlobDataPtr->data();
            // Src data has been converted to be BGR planar format
            int nPixels = w * h * numBlobChannels;
            //for (int i = 0; i < nPixels; i++)
            //    inputDataPtr[i] = src.data[i];
            #if 0
            memcpy(inputDataPtr, src.data, nPixels*sizeof(char));
            #else
            unsigned char *src_buf = src.data;
            std::copy(src_buf, src_buf + nPixels, inputDataPtr);
            #endif
        }
    }else if(InferenceEngine::Precision::FP32 == mInputPrecision){
        InferenceEngine::TBlob<float>::Ptr inputBlobDataPtr = 
            std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(inputBlobPtr);
        if (inputBlobDataPtr != nullptr) {
            float *inputDataPtr = inputBlobDataPtr->data();

            // Src data has been converted to be BGR planar format
            int nPixels = w * h * numBlobChannels;
            #if 1
            // hot code, SSE optimized
            data_norm_SSE(inputDataPtr, src.data, nPixels,  mInputScale, mInputMean);
            #else
            data_norm_C(inputDataPtr, src.data, nPixels,  mInputScale, mInputMean);
            #endif
        }
    }else {
        GST_ERROR("InferenceEngine::Precision not support: %d", (int)mInputPrecision);
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

GstFlowReturn IELoader::second_input_to_blob(InferenceEngine::Blob::Ptr& inputBlobPtr)
{
    //Improvement need: make it can support any data type, and provide a input data pointer
    if( (mInputPrecision != InferenceEngine::Precision::FP32) ||
         (mSecDataPrecision !=  InferenceEngine::Precision::FP32)){
        GST_ERROR("error: second input data is should be FP32!\n");
        return GST_FLOW_ERROR;
    }

    InferenceEngine::TBlob<float>::Ptr inputSecondBlobPtr =
            std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(inputBlobPtr);
    float * inputDataPtr = inputSecondBlobPtr->data();
    float  *src = (float *)mSecDataSrcPtr;
    std::copy(src, src + mSecDataSrcCount, inputDataPtr);
    //g_print("input sencond data!\n");
    return GST_FLOW_OK;
}
int IELoader::get_enable_request()
{
    std::unique_lock<std::mutex> lk(mRequstMutex);
    int target_id = -1;

    mCondVar.wait(lk, [this, &target_id]{
        for(int i = 0; i< REQUEST_NUM; i++) {
            if (mRequestEnable[i]) {
                target_id = i;
                return true;
            }
        }
        return false;
    });

    mRequestEnable[target_id] = false;
    return target_id;
}

void IELoader::release_request(int reqestId)
{
    if(reqestId>=0) {
        std::unique_lock<std::mutex> lk(mRequstMutex);
        mRequestEnable[reqestId] = true;
        mCondVar.notify_all();
    }
}
GstFlowReturn IELoader::get_input_size(int *w, int *h, int *c)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    InferenceEngine::ResponseDesc resp;

    int reqestId = get_enable_request();
    if(reqestId >= 0)
    {
        InferenceEngine::IInferRequest::Ptr inferRequestAsyn = mInferRequest[reqestId];
        InferenceEngine::Blob::Ptr inputBlobPtr;
        IECALLCHECK(inferRequestAsyn->GetBlob(mFirstInputName.c_str(), inputBlobPtr, &resp));
        *w = (int)inputBlobPtr->dims()[0];
        *h = (int)inputBlobPtr->dims()[1];
        *c = (int)inputBlobPtr->dims()[2];
        ret = GST_FLOW_OK;
        release_request(reqestId);
    } else {
        GST_ERROR("Cannot get valid requestid!!!\n");
    }

    return ret;
}

GstFlowReturn IELoader::get_out_size(int *outDim0, int *outDim1)
{
    *outDim1 = (int)mOutputDim[1]; 
    *outDim0 = (int)mOutputDim[0]; 
    return GST_FLOW_ERROR;
}


GstFlowReturn IELoader::do_inference_async(void *data, uint64_t frmId, int objId,
                                                  cv::UMat &src, AsyncCallback cb)
{
    InferenceEngine::ResponseDesc resp;
    CvdlAlgoData *algoData = static_cast<CvdlAlgoData*> (data);

    int reqestId = get_enable_request();
    if(reqestId >= 0)
    {
        InferenceEngine::IInferRequest::Ptr inferRequestAsyn = mInferRequest[reqestId];
        InferenceEngine::Blob::Ptr inputBlobPtr;
        IECALLCHECK(inferRequestAsyn->GetBlob(mFirstInputName.c_str(), inputBlobPtr, &resp));

        // Load images.
        if (src.empty()) {
            g_print("input image empty!!!");
            release_request(reqestId);
            return GST_FLOW_ERROR;
        }
        convert_input_to_blob(src, inputBlobPtr);

        // set data for second input blob
        if(mNeedSecondInputData) {
            InferenceEngine::Blob::Ptr inputBlobPtrSecond;
            IECALLCHECK(inferRequestAsyn->GetBlob(mSecondInputName.c_str(), inputBlobPtrSecond, &resp));
            if (!inputBlobPtrSecond){
                release_request(reqestId);
                g_print("inputBlobPtrSecond is null!\n");
                return GST_FLOW_ERROR;
            }
            second_input_to_blob(inputBlobPtrSecond);
         }

        algoData->ie_start = g_get_monotonic_time();
        // send a request
        IECALLCHECK(inferRequestAsyn->StartAsync(&resp));

        // Start thread listen to result
        auto WaitAsync = [this, algoData, frmId, objId, cb](InferenceEngine::IInferRequest::Ptr inferRequestAsyn, int reqestId)
        {
            //CvdlAlgoBase *algo;
            InferenceEngine::ResponseDesc resp;
            IECALLNORETCHECK(inferRequestAsyn->Wait(InferenceEngine::IInferRequest::WaitMode::RESULT_READY, &resp));
            algoData->ie_duration = g_get_monotonic_time() - algoData->ie_start;
            int duration = algoData->ie_duration/1000;
            GST_INFO("%s: IE wait for %d ms\n", algoData->algoBase->mName.c_str(), duration);
            InferenceEngine::Blob::Ptr resultBlobPtr;
            IECALLNORETCHECK(inferRequestAsyn->GetBlob(mFirstOutputName.c_str(), resultBlobPtr, &resp));

            if (this->mOutputPrecision == InferenceEngine::Precision::FP32)
            {
                CvdlAlgoBase *algo = algoData->algoBase;
                GST_LOG("WaitAsync - do_inference_async begin: algo = %p(%p), algoData = %p\n",
                    algo, algoData->algoBase, algoData);
                //avoid race condition when push object
                algo->mAlgoDataMutex.lock();
                algo->parse_inference_result(resultBlobPtr, sizeof(float), algoData, objId);
                algo->mAlgoDataMutex.unlock();
                GST_LOG("WaitAsync - do_inference_async finish: algo = %p(%p), algoData = %p\n",
                    algo,algoData->algoBase, algoData);
            } else {
                GST_ERROR("Don't support other output precision except FP32!");
                release_request(reqestId);
                return;
            }
            GST_LOG("Got inference result: frmId = %ld",frmId);
            // put result into out_queue
            cb(algoData);
            release_request(reqestId);
        };

        //WaitAsync(inferRequestAsyn, reqestId);
        std::thread t1(WaitAsync, inferRequestAsyn, reqestId);
        t1.detach();
    }

    return GST_FLOW_OK;
}

GstFlowReturn IELoader::do_inference_sync(void *data, uint64_t frmId, int objId,
                                                  cv::UMat &src)
{
    InferenceEngine::StatusCode ret;
    InferenceEngine::ResponseDesc resp;
    CvdlAlgoData *algoData = static_cast<CvdlAlgoData*> (data);

    int reqestId = get_enable_request();
    if(reqestId >= 0)
    {
        InferenceEngine::IInferRequest::Ptr inferRequestSync = mInferRequest[reqestId];
        InferenceEngine::Blob::Ptr inputBlobPtr;
        IECALLCHECK(inferRequestSync->GetBlob(mFirstInputName.c_str(), inputBlobPtr, &resp));

        // Load images.
        if (src.empty()) {
            release_request(reqestId);
            GST_ERROR("input image empty!!!");
            return GST_FLOW_ERROR;
        }
        convert_input_to_blob(src, inputBlobPtr);
         // set data for second input blob
        if(mNeedSecondInputData) {
            InferenceEngine::Blob::Ptr inputBlobPtrSecond;
            IECALLCHECK(inferRequestSync->GetBlob(mSecondInputName.c_str(), inputBlobPtrSecond, &resp));
            if (!inputBlobPtrSecond){
                release_request(reqestId);
                g_print("inputBlobPtrSecond is null!\n");
                return GST_FLOW_ERROR;
            }
            second_input_to_blob(inputBlobPtrSecond);
         }
         ret = inferRequestSync->Infer(&resp);
         if (ret == InferenceEngine::StatusCode::OK){
                InferenceEngine::Blob::Ptr resultBlobPtr;
                ret = inferRequestSync->GetBlob(mFirstOutputName.c_str(), resultBlobPtr, &resp);
                if(ret == InferenceEngine::StatusCode::OK && resultBlobPtr){
                    CvdlAlgoBase *algo = algoData->algoBase;
                    algo->parse_inference_result(resultBlobPtr, sizeof(float), algoData, objId);
                }
            }
            release_request(reqestId);
        }
        return GST_FLOW_OK;
}
