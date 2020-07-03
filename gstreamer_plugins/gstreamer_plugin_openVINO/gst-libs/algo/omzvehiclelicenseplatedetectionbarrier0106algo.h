#ifndef __OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106_ALGO__
#define __OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106_ALGO__

#include "algobase.h"

class OMZVehicleLicensePlateDetectionBarrier0106Algo : public CvdlAlgoBase
{
public:
	OMZVehicleLicensePlateDetectionBarrier0106Algo();
	virtual ~OMZVehicleLicensePlateDetectionBarrier0106Algo();
    virtual GstFlowReturn algo_dl_init(const char* modeFileName) override;
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId) override;
};
#endif
