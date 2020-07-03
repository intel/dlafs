#include "omzvehicleattributesrecognitionbarrier0039algo.h"
#include "algoregister.h"

static void post_callback(CvdlAlgoData *algoData)
{
	// post process algoData
}

OMZVehicleAttributesRecognitionBarrier0039Algo::OMZVehicleAttributesRecognitionBarrier0039Algo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mName = std::string(ALGO_OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039_NAME);
}

OMZVehicleAttributesRecognitionBarrier0039Algo::~OMZVehicleAttributesRecognitionBarrier0039Algo()
{
    g_print("OMZVehicleAttributesRecognitionBarrier0039Algo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0 * mFrameDoneNum / mImageProcCost, 
        1000000.0 * mFrameDoneNum / mInferCost);
}

GstFlowReturn OMZVehicleAttributesRecognitionBarrier0039Algo::algo_dl_init(const char* modeFileName)
{
	GstFlowReturn ret = GST_FLOW_OK;
	//mIeLoader.set_precision(InferenceEngine::Precision::U8, InferenceEngine::Precision::FP32);
	ret = init_ieloader(modeFileName, IE_MODEL_OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039);
	return ret;
}

GstFlowReturn OMZVehicleAttributesRecognitionBarrier0039Algo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("OMZVehicleAttributesRecognitionBarrier0039Algo::parse_inference_result begin: outData = %p\n", outData);
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }
	auto colorsValues =	resultBlobPtr->buffer().as<float*>();
	const auto color_id = std::max_element(colorsValues, colorsValues + 7) - colorsValues;
    ObjectData objData = outData->mObjectVecIn[objId];
	objData.label = mColors[color_id];
	outData->mObjectVec.push_back(objData);
    return GST_FLOW_OK;
}

const std::string OMZVehicleAttributesRecognitionBarrier0039Algo::mColors[] =
{
    "white", "gray", "yellow", "red", "green", "blue", "black"
};

