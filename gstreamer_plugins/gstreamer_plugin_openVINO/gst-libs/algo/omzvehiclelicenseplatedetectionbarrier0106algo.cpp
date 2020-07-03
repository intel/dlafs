#include "omzvehiclelicenseplatedetectionbarrier0106algo.h"
#include "algoregister.h"

static void post_callback(CvdlAlgoData *algoData)
{
	// post process algoData
}

OMZVehicleLicensePlateDetectionBarrier0106Algo::OMZVehicleLicensePlateDetectionBarrier0106Algo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mName = std::string(ALGO_OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106_NAME);
}

OMZVehicleLicensePlateDetectionBarrier0106Algo::~OMZVehicleLicensePlateDetectionBarrier0106Algo()
{
    g_print("OMZVehicleLicensePlateDetectionBarrier0106Algo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0 * mFrameDoneNum / mImageProcCost, 
        1000000.0 * mFrameDoneNum / mInferCost);
}

GstFlowReturn OMZVehicleLicensePlateDetectionBarrier0106Algo::algo_dl_init(const char* modeFileName)
{
	GstFlowReturn ret = GST_FLOW_OK;
	//mIeLoader.set_precision(InferenceEngine::Precision::U8, InferenceEngine::Precision::FP32);
	ret = init_ieloader(modeFileName, IE_MODEL_OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106);
	return ret;
}

GstFlowReturn OMZVehicleLicensePlateDetectionBarrier0106Algo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("OMZVehicleLicensePlateDetectionBarrier0106Algo::parse_inference_result begin: outData = %p\n", outData);
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }
	const float* detection = static_cast<InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type*>(resultBlobPtr->buffer());
    int objectNum = 0;
    int orignalVideoWidth = mImageProcessorInVideoWidth;
    int orignalVideoHeight = mImageProcessorInVideoHeight;
    outData->mObjectVec.clear();
	for (int ix = 0; ix < 5; ++ix)
	{
		auto image_id = static_cast<int>(detection[ix * 7 + 0]);
		if (image_id < 0)
			break;
		float confidence = detection[ix * 7 + 2];
		auto label = static_cast<int>(detection[ix * 7 + 1]);
		if (confidence > 0.5 && label == 1)
		{
			ObjectData object;
            object.id = objectNum;
            object.objectClass = 0;
            object.label = std::string("");
            object.prob = confidence;
			auto xmin =  static_cast<int>(detection[ix * 7 + 3] * orignalVideoWidth);
			auto ymin =  static_cast<int>(detection[ix * 7 + 4] * orignalVideoHeight);
			auto xmax =  static_cast<int>(detection[ix * 7 + 5] * orignalVideoWidth);
			auto ymax =  static_cast<int>(detection[ix * 7 + 6] * orignalVideoHeight);
			xmin = xmin > 0 ? xmin : 0;
			ymin = ymin > 0 ? ymin : 0;
			xmax = xmax > 0 ? xmax : 0;
			ymax = ymax > 0 ? ymax : 0;
			xmin = xmin < orignalVideoWidth - 1 ? xmin : orignalVideoWidth - 1;
			ymin = ymin < orignalVideoHeight - 1 ? ymin : orignalVideoHeight - 1;
			xmax = xmax < orignalVideoWidth - 1 ? xmax : orignalVideoWidth - 1;
			ymax = ymax < orignalVideoHeight - 1 ? ymax : orignalVideoHeight - 1;
			if (xmax - xmin < 0)
				continue;
			if (ymax - ymin < 0)
				continue;
            object.rect = cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
            object.rectROI =cv::Rect(object.rect.x, object.rect.y + object.rect.height / 2, object.rect.width, object.rect.height / 2);
            outData->mObjectVec.push_back(object);
            objectNum++;
		}
	}
    return GST_FLOW_OK;
}
