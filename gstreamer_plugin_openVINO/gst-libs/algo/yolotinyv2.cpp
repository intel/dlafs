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
#include "yolotinyv2.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>

using namespace HDDLStreamFilter;
using namespace std;


static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
}

YoloTinyv2Algo::YoloTinyv2Algo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    set_default_label_name();
}

YoloTinyv2Algo::~YoloTinyv2Algo()
{
    g_print("YoloTinyv2Algo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

void YoloTinyv2Algo::set_default_label_name()
{
    static const char* VOC_LABEL_MAPPING[] = {
        "aeroplane",
        "bicycle",
        "bird",
        "boat",
        "bottle",
        "bus",
        "car",
        "cat",
        "chair",
        "cow",
        "diningtable",
        "dog",
        "horse",
        "motorbike",
        "person",
        "pottedplant",
        "sheep",
        "sofa",
        "train",
        "tvmonitor"
    };
    mLabelNames = VOC_LABEL_MAPPING;
}

void YoloTinyv2Algo::set_data_caps(GstCaps *incaps)
{
    // load IE and cnn model
    std::string filenameXML;
    const gchar *env = g_getenv("CVDL_YOLO_TINY_V2_MODEL_FULL_PATH");
    if(env){
        filenameXML = std::string(env);
    }else{
        filenameXML = std::string(CVDL_MODEL_DIR_DEFAULT"/YoloTinyv2/yolov2-tiny-voc_internaldoc.xml");
    }
    algo_dl_init(filenameXML.c_str());
    init_dl_caps(incaps);
}

GstFlowReturn YoloTinyv2Algo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    // set in/out precision
    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(0.0f,  1.0f);
    ret = init_ieloader(modeFileName, IE_MODEL_YOLOTINYV2);

    return ret;
}

GstFlowReturn YoloTinyv2Algo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("YoloTinyv2Algo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    //parse inference result and put them into algoData
    parse(input, outData);

    return GST_FLOW_OK;
}
    
  /**************************************************************************
  *
  *    private method
  *
  **************************************************************************/
static int argMax(float * data, int num, float * maxValue)
{
    float max = data[0];
    int maxIdx = 0;

    for(int i = 1; i < num; i ++){
        if(data[i] > max){
            maxIdx = i;
            max = data[i];
        }
    }

    *maxValue = max;
    return maxIdx;
}

static bool boxCompare(const YoloBox & box1, const YoloBox & box2)
{
    return box1.objectness > box2.objectness;
}


static float boxIOU(const YoloBox & box1, const YoloBox & box2)
{
    float minX = std::min(box1.x1, box2.x1);
    float maxX = std::max(box1.x2, box2.x2);
    float minY = std::min(box1.y1, box2.y1);
    float maxY = std::max(box1.y2, box2.y2);

    float w1 = box1.x2 - box1.x1;
    float w2 = box2.x2 - box2.x1;

    float h1 = box1.y2 - box1.y1;
    float h2 = box2.y2 - box2.y1;

    float interW = w1 + w2 - (maxX - minX);
    if(interW <= 0){
        return 0;
    }

    float interH = h1 + h2 - (maxY - minY);
    if(interH <= 0){
        return 0;
    }

    float interArea = interH * interW;
    float unionArea = (h1 * w1) + (h2 * w2) - interArea;

    if(unionArea <= 0){
        return 0;
    }

    return interArea / unionArea;
}

static void applyNMS(std::vector<YoloBox> & boxes, float nmsThreshold)
{
    std::sort(boxes.begin(), boxes.end(), boxCompare);

    for(int i = 0; i < (int)boxes.size(); i ++){
        if(boxes[i].objectness < 0){
            continue;
        }

        for(int j = i + 1; j < (int)boxes.size(); j ++){
            if(boxes[j].objectness < 0){
                continue;
            }

            if (boxIOU(boxes[i], boxes[j]) > nmsThreshold){
                boxes[j].objectness = -1.0f;
            }
        }
    }

    auto iter = boxes.begin();
    while(iter != boxes.end()){
        if(iter->objectness < 0){
            iter = boxes.erase(iter);
        }else{
            iter ++;
        }
    }
}

static YoloBox getBox(const float * boxData,
                  int gridWidth, int gridHeight,
                  int anchorWidth, int anchorHeight,
                  int i, int j)
{

    float data[25];
    int channelStride = gridHeight * gridWidth;

    for(int ch = 0; ch < 25; ch ++){
        data[ch] = boxData[ch * channelStride];
    }

    float cx = data[0];
    float cy = data[1];

    float w = data[2];
    float h = data[3];

    float objectConf = data[4];

    float prob;
    int classId = argMax(data + 5, 20, & prob);

    YoloBox box;
    box.classId = classId;
    box.prob = prob * objectConf;
    box.objectness = objectConf;

    cx = (cx + (float)j) / (float)gridWidth;
    cy = (cy + (float)i) / (float)gridHeight;
    w = std::exp(w) * anchorWidth / (float)gridWidth;
    h = std::exp(h) * anchorHeight / (float)gridHeight;

    box.x1 = cx - w / 2.0f;
    box.y1 = cy - h / 2.0f;
    box.x2 = cx + w / 2.0f;
    box.y2 = cy + h / 2.0f;

    box.x2 = box.x2<1.0 ? box.x2 : 0.9999;
    box.y2 = box.y2<1.0 ? box.y2 : 0.9999;

    return box;
}

static bool roiValid(cv::Rect roi, int cols, int rows)
{
    if(roi.x < 0 || roi.x >= cols) return false;
    if(roi.y < 0 || roi.y >= rows) return false;

    cv::Point2i pt2(roi.x + roi.width - 1, roi.y + roi.height - 1);
    if(pt2.x < 0 || pt2.x >= cols) return false;
    if(pt2.y < 0 || pt2.y >= rows) return false;
    return true;
}

bool YoloTinyv2Algo::parse (const float * output, CvdlAlgoData* &outData)
{
    std::vector<YoloBox> boxes;
    int anchorStride = mGridHeight * mGridWidth * 25;
    static const float anchors[10] = {1.08f, 1.19f, 3.42f, 3.41f, 6.63f, 11.38f, 9.42f, 5.11f, 16.62f, 10.52f};
    static const int anchorNum = 5;

    for(int anchorId = 0; anchorId < anchorNum; anchorId ++){
        const float * dataCurrentAnchor = output + anchorStride * anchorId;
        float anchorWidth = anchors[anchorId * 2];
        float anchorHeight = anchors[anchorId * 2 + 1];

        for(int i = 0; i < mGridHeight; i ++){
            for(int j = 0; j < mGridWidth; j ++){
                const float * boxData = dataCurrentAnchor + i * mGridWidth + j;
                YoloBox box = getBox(boxData, mGridWidth, mGridHeight, anchorWidth, anchorHeight, i, j);

                if(box.objectness >= 0.5){
                    boxes.push_back(box);
                }
            }
        }
    }

    float nmsThresh = 0.3f;
    applyNMS(boxes, nmsThresh);

    // write out
    float probThresh = 0.7f;
    int objectNum=0;

    for(auto box: boxes){

        if(box.classId != 14 || box.prob < probThresh){
            continue;
        }

        cv::Rect rect;
        rect.x = (int)(box.x1 * (float)mImageProcessorInVideoWidth);
        rect.y = (int)(box.y1 * (float)mImageProcessorInVideoHeight);
        rect.width = (int)((box.x2 - box.x1) * (float)mImageProcessorInVideoWidth);
        rect.height = (int)((box.y2 - box.y1) * (float)mImageProcessorInVideoHeight);

        if(! roiValid(rect, mImageProcessorInVideoWidth, mImageProcessorInVideoHeight)){
            continue;
        }

        ObjectData object;
        object.id = objectNum;
        object.objectClass = box.classId;
        object.label = std::string(mLabelNames[box.classId]);
        object.prob = box.prob;
        object.rect = rect;
        object.rectROI = rect;
        objectNum++;
 
        outData->mObjectVec.push_back(object);
        GST_LOG("SSD %ld-%d: prob = %f, label = %s, rect=(%d,%d)-(%dx%d)\n",outData->mFrameId, objectNum,
            object.prob, object.label.c_str(),  rect.x,  rect.y,  rect.width,  rect.height);
    }
    return true;
}

