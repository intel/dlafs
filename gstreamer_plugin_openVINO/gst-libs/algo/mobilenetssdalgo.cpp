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

/*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */


#include <string>
#include "mobilenetssdalgo.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>
#include "algoregister.h"

using namespace HDDLStreamFilter;
using namespace std;

static const char* VOC_LABEL_MAPPING[] = {
        "background",    "aeroplane",    "bicycle",    "bird",    "boat",    "bottle",
        "bus",    "car",    "cat",    "chair",    "cow",    "diningtable",    "dog",    "horse",
        "motorbike",    "person",    "pottedplant",    "sheep",    "sofa",    "train",
        "tvmonitor"
    };
static const std::vector<std::string> TRACKING_CLASSES = {
    "person",    "bus",    "car"
};

static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
}
MobileNetSSDAlgo::MobileNetSSDAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mName = std::string(ALGO_MOBILENET_SSD_NAME);
    set_default_label_name();
}

MobileNetSSDAlgo::~MobileNetSSDAlgo()
{
    g_print("MobileNetSSDAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

#if 0
void MobileNetSSDAlgo::set_data_caps(GstCaps *incaps)
{
    std::string filenameXML;
    const gchar *env = g_getenv("HDDLS_CVDL_MODEL_PATH");
    if(env) {
        //($HDDLS_CVDL_MODEL_PATH)/<model_name>/<model_name>.xml
        filenameXML = std::string(env) + std::string("/") + mName + std::string("/") + mName + std::string(".xml");
    }else{
        filenameXML = std::string("HDDLS_CVDL_MODEL_PATH") + std::string("/") + mName
                                  + std::string("/") + mName + std::string(".xml");
        g_print("Error: cannot find %s model files: %s\n", mName.c_str(), filenameXML.c_str());
        exit(1);
    }
    algo_dl_init(filenameXML.c_str());
    init_dl_caps(incaps);
}
#endif


GstFlowReturn MobileNetSSDAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(127.0f,  1.0/127.0f);
    ret = init_ieloader(modeFileName, IE_MODEL_SSD);    
    if(ret != GST_FLOW_OK){
        GST_ERROR("IELoder failed to read model!");
        return GST_FLOW_ERROR;
    }

    mIeLoader.get_out_size(&mSSDObjectSize, &mSSDMaxProposalCount);
    if (mSSDObjectSize != 7) {
            GST_ERROR("SSD object item should have 7 as a last dimension");
            return GST_FLOW_ERROR;
   }
    return ret;
}

GstFlowReturn MobileNetSSDAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("MobileNetSSDAlgo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }
    get_result(input, outData);

    return GST_FLOW_OK;
}

/**************************************************************************
  *
  *    private method
  *
  **************************************************************************/
bool MobileNetSSDAlgo::get_result(float * box,CvdlAlgoData* &outData)
{
     int objectNum = 0;
    outData->mObjectVec.clear();
    for (int i = 0; i < mSSDMaxProposalCount; i++) {
        float image_id = box[i * mSSDObjectSize + 0];

        if (image_id < 0 /* better than check == -1 */) {
            break;
        }

        auto label = (int)box[i * mSSDObjectSize + 1];
        float confidence = box[i * mSSDObjectSize + 2];

        if (confidence < 0.2) {
            continue;
        }

       const char * labelName = mLabelNames[label];
        if(std::find(TRACKING_CLASSES.begin(), TRACKING_CLASSES.end(), labelName) == TRACKING_CLASSES.end()){
            continue;
        }

        auto xmin = (int)(box[i * mSSDObjectSize + 3] * (float)mImageProcessorInVideoWidth);
        auto ymin = (int)(box[i * mSSDObjectSize + 4] * (float)mImageProcessorInVideoHeight);
        auto xmax = (int)(box[i * mSSDObjectSize + 5] * (float)mImageProcessorInVideoWidth);
        auto ymax = (int)(box[i * mSSDObjectSize + 6] * (float)mImageProcessorInVideoHeight);

        if(xmin < 0) xmin = 0; if(xmin >= mImageProcessorInVideoWidth) xmin = mImageProcessorInVideoWidth - 1;
        if(xmax < 0) xmax = 0; if(xmax >= mImageProcessorInVideoWidth) xmax = mImageProcessorInVideoWidth - 1;
        if(ymin < 0) ymin = 0; if(ymin >= mImageProcessorInVideoHeight) ymin = mImageProcessorInVideoHeight - 1;
        if(ymax < 0) ymax = 0; if(ymax >= mImageProcessorInVideoHeight) ymax = mImageProcessorInVideoHeight - 1;

        if(xmin >= xmax || ymin >= ymax) continue;

        // only detect the lower 3/2 part in the image
        if (ymin < (int)((float)mImageProcessorInVideoHeight * 0.1f)) continue;
        if (ymax >= mImageProcessorInVideoHeight -10 ) continue;

        ObjectData object;
        object.id = objectNum;
        object.objectClass = label;
        object.label = std::string(labelName);
        object.prob = confidence;
        object.rect = cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
        object.rectROI = cv::Rect(-1,-1,-1,-1); //for License Plate
        objectNum++;
 
        outData->mObjectVec.push_back(object);
        GST_LOG("SSD %ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n",outData->mFrameId, i,
            object.prob, object.label.c_str(), xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
    }
    return true;
}

void MobileNetSSDAlgo::set_default_label_name()
{
    set_label_names(VOC_LABEL_MAPPING);
}

void MobileNetSSDAlgo::set_label_names(const char** label_names)
{
    mLabelNames = label_names;
}

