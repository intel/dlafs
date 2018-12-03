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

const static int mSSDMaxProposalCount = 100;
const static int mSSDObjectSize=7;

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
         static const char *TRACKING_CLASSES[] = {
             "person",    "bus",    "car"
         };

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
            if(strncmp(labelName, TRACKING_CLASSES[0],strlen(labelName))  &&
                strncmp(labelName, TRACKING_CLASSES[1],strlen(labelName))  &&
                strncmp(labelName, TRACKING_CLASSES[2],strlen(labelName)) ) {
                continue;
            }

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
        return exInferData;
}

// post process all objects for this frame
// return 1  - something has been changed
// return 0  - nothing has been changed
int  post_process_inference_data(ExInferData *data)
{
     //TODO
     return 0;
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