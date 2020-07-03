#ifndef __OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039_ALGO__
#define __OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039_ALGO__

#include "algobase.h"

class OMZVehicleAttributesRecognitionBarrier0039Algo : public CvdlAlgoBase
{
public:
	OMZVehicleAttributesRecognitionBarrier0039Algo();
	virtual ~OMZVehicleAttributesRecognitionBarrier0039Algo();
    virtual GstFlowReturn algo_dl_init(const char* modeFileName) override;
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId) override;
private:
	static const std::string mColors[];
};
#endif
