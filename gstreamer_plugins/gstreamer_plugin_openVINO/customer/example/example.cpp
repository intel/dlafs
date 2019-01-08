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
#include <vector>
#include <algorithm>
#include <cstring>
#include "exinferdata.h"
#include "exinferdata.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

// parser the inference data for one object
ExInferData* parse_inference_result(void *in_data, int data_type, int data_len, int image_width, int image_height)
{
        ExInferData *exInferData = new ExInferData;
        float *box = (float *)in_data;

        static const char* VOC_LABEL_MAPPING[] = {
                 "background",    "aeroplane",    "bicycle",    "bird",    "boat",    "bottle",
                 "bus",    "car",    "cat",    "chair",    "cow",    "diningtable",    "dog",    "horse",
                 "motorbike",    "person",    "pottedplant",    "sheep",    "sofa",    "train",
                 "tvmonitor"
         };
        const static int mSSDMaxProposalCount = 100;
        const static int mSSDObjectSize=7;
        //static const std::vector<std::string> TRACKING_CLASSES = {
        //    "person",    "bus",    "car"
        //};

         int objectNum = 0;
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
    
           const char * labelName = VOC_LABEL_MAPPING[label];
           //if(std::find(TRACKING_CLASSES.begin(), TRACKING_CLASSES.end(), labelName) == TRACKING_CLASSES.end()){
           //      continue;
           //}
           if((label != 6) &&(label!=7) &&(label!=15))
                continue;

            auto xmin = (int)(box[i * mSSDObjectSize + 3] * (float)image_width);
            auto ymin = (int)(box[i * mSSDObjectSize + 4] * (float)image_height);
            auto xmax = (int)(box[i * mSSDObjectSize + 5] * (float)image_width);
            auto ymax = (int)(box[i * mSSDObjectSize + 6] * (float)image_height);
    
            if(xmin < 0) xmin = 0; if(xmin >= image_width) xmin = image_width - 1;
            if(xmax < 0) xmax = 0; if(xmax >= image_width) xmax = image_width - 1;
            if(ymin < 0) ymin = 0; if(ymin >= image_height) ymin = image_height - 1;
            if(ymax < 0) ymax = 0; if(ymax >= image_height) ymax = image_height - 1;
    
            if(xmin >= xmax || ymin >= ymax) continue;
    
            // only detect the lower 3/2 part in the image
            if (ymin < (int)((float)image_height * 0.1f)) continue;
            if (ymax >= image_height -10 ) continue;
    
            ExObjectData object;
            object.id = objectNum;
            object.objectClass = label;
            object.label = std::string(labelName);
            object.prob = confidence;
            object.x = xmin;
            object.y = ymin;
            object.w = xmax - xmin + 1;
            object.h = ymax - ymin + 1;
            //object.rect = cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
            //object.rectROI = cv::Rect(-1,-1,-1,-1); //for License Plate
            objectNum++;
     
            exInferData->mObjectVec.push_back(object);
            printf("Generic SSD %d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n", i,
                object.prob, object.label.c_str(), xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
        }
        exInferData->outputIndex = 0; //must set its value
        return exInferData;
}

// post process all objects for this frame
// return 1  - something has been changed
// return 0  - nothing has been changed
int  post_process_inference_data(ExInferData *data)
{
     //TODO
     data->outputIndex = 0; //must set its value
     return 0;
}

const char *get_network_config(const char* modelName)
{
    std::string modelNameStr = std::string(modelName);
    std::string config_xml =  std::string("file=") + modelNameStr.substr(0, modelNameStr.rfind(".")) + std::string(".conf.xml");
    char* config = strdup(config_xml.c_str());
    return config;
}

// get the data type for inference input and result
void get_data_type(ExDataType *inData,  ExDataType *outData)
{
     *inData = DataTypeFP32;
     *outData = DataTypeFP32;
}
void get_mean_scale(float *mean, float *scale)
{
    *mean = 127.0f;
    *scale = 1.0/127.0f;
}
#ifdef __cplusplus
};
#endif
