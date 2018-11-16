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

 /*
  * Track car/bus and detect the license plate
  */
#include <ocl/oclmemory.h>
#include "mathutils.h"
#include "tracklpalgo.h"
#include <interface/videodefs.h>
#include "dlib/optimization.h"

using namespace cv;
using namespace HDDLStreamFilter;

#define CHECK_ROI(rt, W, H) \
        if(rt.x < 0 || rt.y < 0 || rt.x + rt.width > W || rt.y + rt.height > H ||   \
           rt.width < 0 || rt.height < 0){  \
            g_print("check roi = [%d,%d,%d,%d]\n", rt.x, rt.y, rt.width, rt.height); \
            std::exit(-1);    \
        }


#define TRACKING_LP_INPUT_W 300
#define TRACKING_LP_INPUT_H 300

/**
 * If not detect number >= TRACK_MAX_NUM, tracking will stop.
 */
#define TRACK_LP_MAX_NUM 8
#define TRACK_LP_FRAME_NUM 2

#define FLAGS_TRACKED_LP_DATA_IS_SET   0x1
#define FLAGS_TRACKED_LP_DATA_IS_PASS  0x2

const int MaxOcvPredict = 6;
const int KFMaxAge = 12;

static void parseTrackResult(const BBoxArrayWithId & bboxArrayWithId, std::vector<ObjectData> & trackResult);
static void post_tracklp_process(CvdlAlgoData *algoData)
{
     TrackLpAlgo *trackLpAlgo = static_cast<TrackLpAlgo*> (algoData->algoBase);
     std::set<int> licencePlateInfoExtracted;
     BBoxArrayWithId bboxArrayWithId;
     MathUtils utils;

       // verify the input object data
       trackLpAlgo->verify_detection_result(algoData->mObjectVec);
    
       // If not receive for 3 frame, clear all tracked objects
       if(trackLpAlgo->mFrameIndex - trackLpAlgo->mFrameIndexLast > 3) {
                trackLpAlgo->mTrackObjVec.clear();
       }
       trackLpAlgo->mFrameIndexLast = trackLpAlgo->mFrameIndex;
    
        // get object box 
        auto vecDetectRt = algoData->mObjectVec;
        BBoxArray boxes;
        for(auto & objectData : vecDetectRt){
                // convert it from orignal size base to tracking image base
                cv::Rect2d cvbox = utils.convert_rect(objectData.rect, 
                                     trackLpAlgo->mImageProcessorInVideoWidth,
                                     trackLpAlgo->mImageProcessorInVideoHeight,
                                     trackLpAlgo->mInputWidth, trackLpAlgo->mInputHeight);
                boxes.push_back(convertOcvBBoxToBBox(cvbox));
        }
    
      // track
      std::vector<ObjectData> trackResult;
       bboxArrayWithId = trackLpAlgo->lpTracker->update(boxes, trackLpAlgo->mImageProcessorInVideoWidth, 
            trackLpAlgo->mImageProcessorInVideoHeight);
       // trackResult is the track result, but not used for LP detection
       const std::map<long, long> & boxIDToObjectID = trackLpAlgo->lpTracker->getBoxToObjectMapping();
       parseTrackResult(bboxArrayWithId, trackResult);
       for(guint i=0; i<trackResult.size();i++) {
               trackResult[i].rect = utils.convert_rect(trackResult[i].rect,
                                     trackLpAlgo->mInputWidth, trackLpAlgo->mInputHeight,
                                     trackLpAlgo->mImageProcessorInVideoWidth,
                                     trackLpAlgo->mImageProcessorInVideoHeight);
        }
    
       // loop each object and detect LP
       int lpIdx=0;
       for(guint i=0; i<algoData->mObjectVec.size();i++) {
             ObjectData &objectData = algoData->mObjectVec[i];
             objectData.flags = 0;
             if(strncmp(objectData.label.c_str(), "car", 3) &&
                 strncmp(objectData.label.c_str(), "bus", 3) ) {
                    continue;
             }
    
             cv::Rect vehicleBox = objectData.rect;
             cv::Rect licensePlateBox = objectData.rectROI;
    
             auto iter = boxIDToObjectID.find(i);
             if(iter == boxIDToObjectID.end()){
                    continue;
              }
              //int objectId = (int)(iter->second);
    
              // choose a better object
          #if 0
              float score = objectData.figure_score(trackLpAlgo->mImageProcessorInVideoWidth,
                        trackLpAlgo->mImageProcessorInVideoHeight);
              if(score < 1.0)
                    continue;
          #endif
    
               // get input data and process it here
               // NV12-->BGR
                GstBuffer *ocl_buf = NULL;
                VideoRect crop = {(uint32_t)vehicleBox.x,(uint32_t)vehicleBox.y, (uint32_t)vehicleBox.width, (uint32_t)vehicleBox.height};
                trackLpAlgo->mImageProcessor.process_image(algoData->mGstBuffer,NULL, &ocl_buf, &crop);
                if(ocl_buf==NULL) {
                        g_print("Failed to do image process!\n");
                        trackLpAlgo->mInferCnt=0;
                        return;
                }
                OclMemory *ocl_mem = NULL;
                ocl_mem = ocl_memory_acquire (ocl_buf);
                if(ocl_mem==NULL){
                        g_print("Failed get ocl_mem after image process!\n");
                        trackLpAlgo->mInferCnt=0;
                        return;
                }
    
               // detect LP based on mObjectVec
               cv::Mat roiImage = ocl_mem->frame.getMat(0);
               cv::Rect lpDetResult = trackLpAlgo->lpDetect->detectLicencePlates(roiImage);
               gst_buffer_unref(ocl_buf);
               if(lpDetResult.x < 0){
                    continue;
               }
    
                //test
                /*
                int test_idx = algoData->mFrameId+i;
                trackLpAlgo->save_buffer(ocl_mem->frame.getMat(0).ptr(), trackLpAlgo->mInputWidth,
                    trackLpAlgo->mInputHeight,3,test_idx,0, "track_lp"); 
                g_print("idx=%d, LP=(%d,%d)@%dx%d\n",test_idx, lpDetResult.x, lpDetResult.y, lpDetResult.width, lpDetResult.height);
           */
    
            #if 0
                int adjustX  =  (lpDetResult.x>2) ? lpDetResult.x -2 : lpDetResult.x;
                lpDetResult.width = lpDetResult.width + (lpDetResult.x - adjustX);
                lpDetResult.x = adjustX;
            #endif
    
            #if 0
                // expand LP scope by the trick - LP w/h=4;
                 int adjustW =  lpDetResult.height * 4;// trick- LP w/h=4
                 int adjustX = lpDetResult.x + lpDetResult.width/2 - adjustW/2;
                 if(adjustX<0) {
                      adjustW = adjustW + adjustX;
                      adjustX=0;
                 }
                 if(adjustX+adjustW>trackLpAlgo->mInputWidth) {
                       adjustW = trackLpAlgo->mInputWidth - adjustX - 1;
                 }
                 lpDetResult.x = adjustX;
                 lpDetResult.width = adjustW;
             #endif
    
             #if 1
                 // expand LP scope 10%
                  int adjustW =  lpDetResult.width * 1.16;
                  int adjustH =  lpDetResult.height * 1.1;
                  int adjustX = lpDetResult.x +( lpDetResult.width - adjustW)*3/5;
                  int adjustY = lpDetResult.y + lpDetResult.height - adjustH;
                  if(adjustX<0) {
                       adjustW = adjustW + adjustX;
                       adjustX=0;
                  }
                  if(adjustX+adjustW>trackLpAlgo->mInputWidth) {
                        adjustW = trackLpAlgo->mInputWidth - adjustX - 1;
                  }
                  if(adjustY+adjustH>trackLpAlgo->mInputHeight) {
                        adjustH = trackLpAlgo->mInputHeight - adjustY - 1;
                  }
                  lpDetResult.x = adjustX;
                  lpDetResult.width = adjustW;
                  lpDetResult.y = adjustY;
                  lpDetResult.height = adjustH;
            #endif
     
                // figure out licensePlateBox
                licensePlateBox = lpDetResult;
                licensePlateBox.x = vehicleBox.x  + lpDetResult.x * vehicleBox.width / trackLpAlgo->mInputWidth;
                licensePlateBox.y = vehicleBox.y + lpDetResult.y * vehicleBox.height / trackLpAlgo->mInputHeight;
                licensePlateBox.width  = lpDetResult.width  * vehicleBox.width / trackLpAlgo->mInputWidth;
                licensePlateBox.height = lpDetResult.height  * vehicleBox.height / trackLpAlgo->mInputHeight;
    
                // 4 pixels aligned for LP
                licensePlateBox.x &= ~0x7;
                licensePlateBox.y &= ~0x3;
                licensePlateBox.width = (licensePlateBox.width+7) & (~7);
                licensePlateBox.height = (licensePlateBox.height+3)&(~3);
    
                CHECK_ROI(licensePlateBox,  trackLpAlgo->mImageProcessorInVideoWidth, trackLpAlgo->mImageProcessorInVideoHeight);
                objectData.rectROI = licensePlateBox;
                lpIdx ++;
                objectData.flags = 1;
    
                 trackLpAlgo->try_add_new_one_object(objectData, algoData->mFrameId);
        }
       //trackLpAlgo->print_objects(algoData->mObjectVec);
       trackLpAlgo->remove_no_lp(algoData->mObjectVec);
    
       // detectLicencePlates() has filtered candidate frame to choose the best one
       // so we need do it again
#if  1
         // update mTrackObjVec
        trackLpAlgo->verify_tracked_object();
        trackLpAlgo->update_track_object(algoData->mObjectVec);
        trackLpAlgo->mLastObjectTrackRes = trackResult;
 #endif

}

TrackLpAlgo::TrackLpAlgo():CvdlAlgoBase(post_tracklp_process, CVDL_TYPE_CV)
{
    mInputWidth = TRACKING_LP_INPUT_W;
    mInputHeight = TRACKING_LP_INPUT_H;

    mSvmModelStr = std::string(CVDL_MODEL_DIR_DEFAULT"/svm_model.xml");

    lpTracker = new LpTracker(KFMaxAge);
    lpDetect = new LicencePlateDetect(mSvmModelStr);
}

TrackLpAlgo::~TrackLpAlgo()
{
    delete lpTracker;
    delete lpDetect;
    g_print("TrackLpAlgo: image process %d frames, image preprocess fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost);
}

// set data caps
//   1. pass orignal video caps to it, which is the input data of image processor
//   2. create ocl caps for OCL processing the orignal video image to be the Gray format, which is the input of object tracking
void TrackLpAlgo::set_data_caps(GstCaps *incaps)
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    // set OCL surface: width and height and format
    //   LP track will use BGR data format
    //int oclSize = mInputWidth * mInputHeight * 3;
     mOclCaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    gst_caps_set_simple (mOclCaps, "width", G_TYPE_INT, mInputWidth, "height",
                         G_TYPE_INT, mInputHeight, NULL);

    mImageProcessor.ocl_init(incaps, mOclCaps, IMG_PROC_TYPE_OCL_CRC, CRC_FORMAT_BGR);
    mImageProcessor.get_input_video_size(&mImageProcessorInVideoWidth,
                                         &mImageProcessorInVideoHeight);
    gst_caps_unref (mOclCaps);
}

/**
 * @Brief HDDL detect results could have some errors.
 * eg.x, y, width or height may be < 0, or is too small
 * We will remove the exception results
 * @param vecDetectRt: HDDL detect results
 */
void TrackLpAlgo::verify_detection_result(std::vector<ObjectData> &objectVec)
{
    std::vector<ObjectData> vecObjectCp = objectVec;
    objectVec.clear();
    int orignalVideoWidth = mImageProcessorInVideoWidth;
    int orignalVideoHeight = mImageProcessorInVideoHeight;
    MathUtils utils;

    for(size_t i = 0; i < vecObjectCp.size(); i++){
        cv::Rect& curRt = vecObjectCp[i].rect;
        // Rect can't beyond image edges.
        curRt = utils.verify_rect(orignalVideoWidth, orignalVideoHeight, curRt);

        // If object is small and first occur. remove
        int centY = curRt.y + curRt.height / 2;
        int area = curRt.width * curRt.height;
        int areaThreshold = 40 * 40; /* 40x40 pixels in orignal image*/
        if (centY > orignalVideoHeight * 2 / 3 && area < areaThreshold) {
            continue;
        }

        vecObjectCp[i].flags  &= ~(FLAGS_TRACKED_LP_DATA_IS_PASS & FLAGS_TRACKED_LP_DATA_IS_SET);
        objectVec.push_back(vecObjectCp[i]);
    }

    /**
    *  The same object, may be give out 2 rectangle, we need merge them.
    *  After merge, we choose the later object as last object.
    */
    vecObjectCp = objectVec;
    objectVec.clear();
    int rtNum = vecObjectCp.size();

    for (int i = 0; i < rtNum - 1; i++) {
        float ol12 = 0, ol21 = 0;
        utils.get_overlap_ratio(vecObjectCp[i].rect, vecObjectCp[i + 1].rect, ol12, ol21);
        if (ol12 > 0.65f || ol21 > 0.65f) {
            //merge
            continue;
        } else {
            objectVec.push_back(vecObjectCp[i]);
        }
    }

    if (rtNum > 0) {
        objectVec.push_back(vecObjectCp[rtNum - 1]);
    }
}

void TrackLpAlgo::try_add_new_one_object(ObjectData &objectData, guint64 frameId)
{
    TrackLpObjAttribute obj;
    cv::Rect objRect = objectData.rect;
    MathUtils utils;
    std::vector<TrackLpObjAttribute>::iterator it;
    gboolean bNewOne = TRUE;

   //check new object can match one in mTrackObjVec
   for (it = mTrackObjVec.begin(); it != mTrackObjVec.end();) {
       cv::Rect &rect = (*it).rect;
       float ov12, ov21;
       utils.get_overlap_ratio(rect, objRect, ov12, ov21);
        if ((ov12 > 0.60f || ov21 > 0.4f)  &&  objectData.rect.y > rect.y * 9/10) {
                    bNewOne = FALSE;
                    objectData.flags |= FLAGS_TRACKED_LP_DATA_IS_SET;
                    objectData.id = (*it).objId;
                    (*it).rect = objRect;
                     // We will set them in update_track_object()
                    /* 
                (*it).detectedNum++;
                (*it).notDetectNum = 0;
             */
                     // convert it from orignal size base to tracking image base
                     objRect = utils.convert_rect(objectData.rect, 
                                 mImageProcessorInVideoWidth,
                                 mImageProcessorInVideoHeight,
                                 mInputWidth, mInputHeight);
                     (*it).vecPos.push_back(objRect);
                     (*it).bHit = TRUE;
                    break;
        }
        it++;
   }

   if(bNewOne==FALSE)
        return;

    // a new object will be added.
    obj.objId = mCurObjId;
    obj.score = 0.0;
    obj.detectedNum = 0;
    obj.fliped = false;
    obj.startFrameId = frameId;
    obj.curFrameId = obj.startFrameId;
    obj.bHit = TRUE;
    obj.rect = objRect;

    obj.notDetectNum = 0;
    obj.bBottom = false;

    objectData.flags |= FLAGS_TRACKED_LP_DATA_IS_SET;
    objectData.id =obj.objId;

    VideoPoint point = {objRect.x+objRect.width/2,
                        objRect.y+objRect.height/2};
    objectData.trajectoryPoints.push_back(point);

    // convert it from orignal size base to tracking image base
    objRect = utils.convert_rect(objectData.rect, 
                                 mImageProcessorInVideoWidth,
                                 mImageProcessorInVideoHeight,
                                 mInputWidth, mInputHeight);
    obj.vecPos.push_back(objRect);

    // vec is based on the orignal video size
    CHECK_ROI(objectData.rect, mImageProcessorInVideoWidth, mImageProcessorInVideoHeight);
    mTrackObjVec.push_back(obj);

    mCurObjId++;
}


// Go through each tracked object, if not detected then set notDetectNum++
void TrackLpAlgo::verify_tracked_object()
{
    std::vector<TrackLpObjAttribute>::iterator it;
    for (it = mTrackObjVec.begin(); it != mTrackObjVec.end();) {
        if((*it).bHit  == FALSE) {
                 (*it).notDetectNum++;
                 //(*it).detectedNum=0;
         }
         (*it).bHit = FALSE;
         it++;
    }
}

bool TrackLpAlgo::is_at_buttom(TrackLpObjAttribute& curObj)
{
    cv::Rect rt = curObj.getLastPos();

    if (rt.height < 60 && rt.y + rt.height > mInputHeight - 5) {
        return true;
    }

    return false;
}

void TrackLpAlgo::update_track_object(std::vector<ObjectData> &objectVec)
{
    std::vector<TrackLpObjAttribute>::iterator it;
    float score = 0.0;
    static int id = -1;
    id++;

    //print_objects(objectVec);

    for (it = mTrackObjVec.begin(); it != mTrackObjVec.end();) {
        // find the connected objectData
        for(unsigned int i=0; i<objectVec.size(); i++){
            ObjectData& objectData = objectVec[i];
            if(objectData.id == (*it).objId) {
                score = objectData.figure_score(mImageProcessorInVideoWidth,
                    mImageProcessorInVideoHeight);
                //if(score>1.0) {
                   (*it).detectedNum ++;
                //}
                //g_print("idx = %d, score=%f, it.score = %f, detectedNum =%d, fliped = %d,"
                //     "notDetectNum = %d\n",  id,  score, (*it).score,  (*it).detectedNum,  (*it).fliped, (*it).notDetectNum);
                if((*it).fliped==false) {
                    if( ((score > 2.5) && ( (*it).detectedNum >= 1))
                          || ( (*it).detectedNum >= TRACK_LP_FRAME_NUM)) {
                        objectData.flags |= FLAGS_TRACKED_LP_DATA_IS_PASS;
                        (*it).fliped = true;
                        (*it).detectedNum = 0;
                        objectData.score = (*it).score;
                    }
                }
                (*it).score = score;
                break;
            }
        }

        bool bButtom = is_at_buttom(*it);
        if (it->notDetectNum >= TRACK_LP_MAX_NUM || bButtom) {
            it = mTrackObjVec.erase(it);    //remove item.
            continue;
        } else {
            ++it;
        }
    }


    // remove the objectData which has not been set PASS flag
    std::vector<ObjectData>::iterator obj;
    for (obj = objectVec.begin(); obj != objectVec.end();) {
        if(!((*obj).flags & FLAGS_TRACKED_LP_DATA_IS_PASS)) {
            // remove  it
            obj = objectVec.erase(obj);
        } else {
            ++obj;
        }
    }
}

void TrackLpAlgo::remove_no_lp(std::vector<ObjectData> &objectVec)
{
    std::vector<ObjectData> vecObjectCp = objectVec;
    objectVec.clear();
    int orignalVideoWidth = mImageProcessorInVideoWidth;
    int orignalVideoHeight = mImageProcessorInVideoHeight;
    MathUtils utils;
    
    for(size_t i = 0; i < vecObjectCp.size(); i++){
          cv::Rect& curRt = vecObjectCp[i].rect;
          if(vecObjectCp[i].flags==0)
               continue;
           // Rect can't beyond image edges.
          curRt = utils.verify_rect(orignalVideoWidth, orignalVideoHeight, curRt);
           vecObjectCp[i].score = 0.0;
          objectVec.push_back(vecObjectCp[i]);
      }
}

//-------------------------------------------------------------------------
//
//  Licence Plate Locate
//
//-------------------------------------------------------------------------
struct HSVScope{
    int minH;
    int maxH;
    int minS;
    int maxS;
    int minV;
    int maxV;
};

#if 0
static struct HSVScope hsvScope[] = {
    {100, 140, 95, 255, 95, 255}, // Blue plate
    {15, 40,    95, 255, 95, 255}, // Yellow palte
};
static struct HSVScope hsvScope[] = {
    {90, 120, 80, 220, 80, 255}, // Blue plate
    {11, 34,    43, 255, 46, 255}, // Yellow palte
    {0, 180,       0, 255,   0,   46 }, // black plate
    {0,180,        0,    30, 46, 220} // white plate
};
#else
static struct HSVScope hsvScope[] = {
    {90, 140,  80,  255, 80, 255}, // Blue plate
    {11,    34,  43,  255, 46, 255}, // Yellow palte
    {0,    180,    0,  255,   0,   46 }, // black plate
    {0,    180,    0,    30,  46, 220} // white plate
};

#endif
static bool checkHSV(const cv::Vec3b & hsv)
{
   int i = 0;
   bool ret = false;

   for(i=0;i<2;i++) {
        ret     = (hsv[0] >hsvScope[i].minH) && (hsv[0]<hsvScope[i].maxH);
        ret  &= (hsv[1] >hsvScope[i].minS) && (hsv[1]<hsvScope[i].maxS);
        ret  &= (hsv[2] >hsvScope[i].minV) && (hsv[2]<hsvScope[i].maxV);
        if(ret)
            return true;
    }
    return false;
}


static bool checkSize(cv::RotatedRect rect)
{
    // check aspect and check size
    float aspect = rect.size.width * 1.0f / rect.size.height;
    if(aspect < 1.){
        aspect = rect.size.height * 1.0f / rect.size.width;
    }
    float size = rect.size.height * rect.size.width;

    float deltaAspect = 3.75f - aspect;
    deltaAspect = deltaAspect < 0 ? - deltaAspect: deltaAspect;

    if( deltaAspect / 3.75f > 0.1){
        return false;
    }

    int minSize = 300;
    int maxSize = 6500;
    if (size < minSize){
        return false;
    }
    if (size > maxSize){
        return false;
    }

    return true;
}

static cv::Rect getBoundingBox(cv::RotatedRect rect, cv::Size sz)
{
     cv::Rect boundingBox = rect.boundingRect();
     int x1, y1, x2, y2;

     x1 = boundingBox.x;
     y1 = boundingBox.y;
     x2 = boundingBox.x + boundingBox.width - 1;
     y2 = boundingBox.y + boundingBox.height - 1;

     x1 = x1 < 0? 0: x1;
     y1 = y1 < 0? 0: y1;
     x2 = x2 > sz.width - 1? sz.width - 1: x2;
     y2 = y2 > sz.height - 1? sz.height - 1: y2;

     int newWidth = x2 - x1 + 1;
     int newHeight = y2 - y1 + 1;
     if (newWidth <= 0 || newHeight <= 0){
         return cv::Rect(-1, -1, -1, -1);
     }

     return cv::Rect(x1, y1, newWidth, newHeight);
}

LicencePlateDetect::LicencePlateDetect(std::string svmModelPath)
{
    svmClassifier = cv::ml::SVM::load(svmModelPath);
}

cv::Rect LicencePlateDetect::detectLicencePlates(const cv::Mat & image)
{
    // convert to HSV color space, and search blue & yellow candidate areas.
    cv::Mat hsvImage;
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> hsvChannels;
    cv::split(hsvImage, hsvChannels);
    cv::equalizeHist(hsvChannels[2], hsvChannels[2]);
    cv::merge(hsvChannels, hsvImage);

    cv::Mat candidateMask(hsvImage.rows, hsvImage.cols, CV_8UC1);

    for(int i = 0; i < hsvImage.rows; i ++){
        uchar * rowPtr = hsvImage.ptr(i);
        uchar * rowPtrMask = candidateMask.ptr(i);
        for(int j = 0; j < hsvImage.cols; j ++){
            cv::Vec3b & hsvValue = ((cv::Vec3b*)rowPtr)[j];
            rowPtrMask[j] = checkHSV(hsvValue) ? 255 : 0;
        }
    }

    // apply morphological processing to the mask.
    cv::Mat morphoElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(10, 2));
    cv::morphologyEx(candidateMask, candidateMask, cv::MORPH_CLOSE,  morphoElement);

    // contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(candidateMask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    auto iter = contours.begin();
    std::vector<cv::RotatedRect> candidates;

    while (iter != contours.end()) {
        cv::RotatedRect candidate = cv::minAreaRect(*iter);
        if(checkSize(candidate)){
            candidates.push_back(candidate);
        }
        iter ++;
    }

    // get bounding box
    std::vector<cv::Rect> boundingBoxes;
    for(auto candidate : candidates){
        if (candidate.size.width < candidate.size.height) {
          candidate.angle += 90;
          std::swap(candidate.size.width, candidate.size.height);
        }

        if(candidate.angle >= 15 || candidate.angle  <= -15){
            continue;
        }

        cv::Rect boundingBox = getBoundingBox(candidate, image.size());

        if(boundingBox.width < 0){
            continue;
        }

        boundingBoxes.push_back(boundingBox);
    }

    cv::Rect finalBoundingBox;
    finalBoundingBox.x = finalBoundingBox.y = -1;

    // choose the bounding box with max confidence (min margin)
    auto boxIter = boundingBoxes.begin();
    if (boxIter == boundingBoxes.end()){
        return finalBoundingBox;
    }

    finalBoundingBox = *boxIter;
    float minScore = 100.0f;

    while(boxIter != boundingBoxes.end()){
        auto boundingBox = *boxIter;
        cv::Mat roiImage = image(boundingBox);
        cv::resize(roiImage, roiImage, cv::Size(136, 36));
        cv::Mat features = calcFeatures(roiImage);
        float score = svmClassifier->predict(features, cv::noArray(),
                                             cv::ml::StatModel::Flags::RAW_OUTPUT);
        if( score < minScore){
            minScore = score;
            finalBoundingBox = boundingBox;
        }
        boxIter ++;
    }

    if( minScore >= 0.5){
        finalBoundingBox.x = finalBoundingBox.y = -1;
        finalBoundingBox.height = finalBoundingBox.width = -1;
    }

    return finalBoundingBox;
}


cv::Mat  LicencePlateDetect::calcFeatures(const cv::Mat & plateImage)
{
    // Get histogram features
    cv::Mat gray;
    cv::cvtColor(plateImage, gray, CV_RGB2GRAY);

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);

    int rows = binary.rows;
    int cols = binary.cols;

    cv::Mat hHis(1, rows, CV_32F);
    cv::Mat vHis(1, cols, CV_32F);
    hHis.setTo(0);
    vHis.setTo(0);

    for(int i = 0; i < rows; i ++){
        uchar * rowPtr = binary.ptr(i);
        for(int j = 0; j < cols; j ++){
            if( rowPtr[j] > 20){
                hHis.at<float>(i) += 1;
                vHis.at<float>(j) += 1;
            }
        }
    }

    // normalization
    double min, max;
    cv::minMaxLoc(hHis, &min, &max);
    if (max > 0)
      hHis.convertTo(hHis, -1, 1.0 / max, 0);

    cv::minMaxLoc(vHis, &min, &max);
    if (max > 0)
      vHis.convertTo(vHis, -1, 1.0 / max, 0);

    cv::Mat histFeature(1, rows + cols, CV_32F);

    for(int i = 0; i < cols; i ++){
        histFeature.at<float>(i) = vHis.at<float>(i);
    }
    for(int i = 0; i < rows; i ++){
        histFeature.at<float>(i + cols) = hHis.at<float>(i);
    }

    // Get color features
    cv::Mat hsv;
    cv::cvtColor(plateImage, hsv, cv::COLOR_RGB2HSV);
    cv::Mat colorFeature(1, 180, CV_32F);
    colorFeature.setTo(0);

    for(int i = 0; i < hsv.rows; i ++){
        uchar * rowPtr = hsv.ptr(i);
        for(int j = 0; j < hsv.cols; j ++){
            int hvalue = ((cv::Vec3b*)rowPtr)[j][0];
            hvalue = hvalue >= 180 ? 179: hvalue;
            hvalue = hvalue < 0 ? 0: hvalue;
            colorFeature.at<float>(hvalue) += 1;
        }
    }

   cv::minMaxLoc(colorFeature, &min, &max);
   if(max > 0)
       colorFeature.convertTo(colorFeature, -1, 1.0 / max, 0);

   // combine hist feature and color-hist feature.
   int featureDim = histFeature.cols + colorFeature.cols;

   //std::cout << "feature dim = " << featureDim << std::endl;
   cv::Mat features(1, featureDim, CV_32F);

   int idx = 0;
   for(int i = 0; i < histFeature.cols; i ++){
       features.at<float>(idx) = histFeature.at<float>(i);
       idx += 1;
   }
   for(int i = 0; i < colorFeature.cols; i ++){
       features.at<float>(idx) = colorFeature.at<float>(i);
       idx += 1;
   }

   return features;
}

//-------------------------------------------------------------------------
//
//  KFTrack
//
//-------------------------------------------------------------------------
KFTrack::KFTrack(
    const Eigen::MatrixXd & A,
    const Eigen::MatrixXd & C,
    const Eigen::MatrixXd & Q,
    const Eigen::MatrixXd & R,
    const Eigen::MatrixXd & P,
    const Eigen::VectorXd & initState)
{
    this->A = A;
    this->C = C;
    this->Q = Q;
    this->R = R;
    this->P = P;

    int n = A.rows();
    this->I = Eigen::MatrixXd(n, n);
    this->I.setIdentity();

    this->xHat = initState;
    this->xHatNew = Eigen::VectorXd(n);
}

Eigen::VectorXd KFTrack::predict()
{
    xHatNew = A * xHat;
    P = A*P*A.transpose() + Q;
    return xHatNew;
}

void KFTrack::updateWithPredicted()
{
    xHat = xHatNew;
}

void KFTrack::update(const Eigen::VectorXd& observed)
{
  K = P*C.transpose()*(C*P*C.transpose() + R).inverse();
  xHatNew += K * (observed - C*xHatNew);
  P = (I - K*C)*P;
  xHat = xHatNew;
}

//-------------------------------------------------------------------------
//
//  KalmanBoxTracker && LpTracker
//
//-------------------------------------------------------------------------
static std::vector<double> convert_bbox_to_z(const std::vector<double> & bbox)
{
    std::vector<double> z;
    double w = bbox[2]-bbox[0];
    double h = bbox[3]-bbox[1];
    double x = bbox[0]+w/2;
    double y = bbox[1]+h/2;
    double s = w*h;
    double r = w/h;
    z.push_back(x);
    z.push_back(y);
    z.push_back(s);
    z.push_back(r);
    return z;
}


static std::vector<double> convert_x_to_bbox(const std::vector<double> & x)
{
    double w = sqrt(x[2]*x[3]);
    double h = x[2]/w;
    std::vector<double> bbox;
    bbox.push_back(x[0]-w/2);
    bbox.push_back(x[1]-h/2);
    bbox.push_back(x[0]+w/2);
    bbox.push_back(x[1]+h/2);
    return bbox;
}


KalmanBoxTracker::KalmanBoxTracker(int id, std::vector<double> bbox)
{
    this->timeSinceUpdate = 0;
    this->id = id;
    this->hits = 0;
    this->hitStreak = 0;
    this->dimX = 7;
    this->dimZ = 4;

    int n = this->dimX; // dim x (state)
    int m = this->dimZ; // dim z (measurement)

    Eigen::MatrixXd A(n, n); // System dynamics matrix
    Eigen::MatrixXd C(m, n); // Output matrix
    Eigen::MatrixXd Q(n, n); // Process noise covariance
    Eigen::MatrixXd R(m, m); // Measurement noise covariance
    Eigen::MatrixXd P(n, n); // Estimate error covariance

    A << 1,0,0,0,1,0,0,
         0,1,0,0,0,1,0,
         0,0,1,0,0,0,1,
         0,0,0,1,0,0,0,
         0,0,0,0,1,0,0,
         0,0,0,0,0,1,0,
         0,0,0,0,0,0,1;

    C << 1,0,0,0,0,0,0,
         0,1,0,0,0,0,0,
         0,0,1,0,0,0,0,
         0,0,0,1,0,0,0;

    Q << 1.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.    ,
         0.    ,  1.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.    ,
         0.    ,  0.    ,  1.    ,  0.    ,  0.    ,  0.    ,  0.    ,
         0.    ,  0.    ,  0.    ,  1.    ,  0.    ,  0.    ,  0.    ,
         0.    ,  0.    ,  0.    ,  0.    ,  0.01  ,  0.    ,  0.    ,
         0.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.01  ,  0.    ,
         0.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.    ,  0.0001;

    R << 1.,   0.,   0.,   0.,
         0.,   1.,   0.,   0.,
         0.,   0.,  10.,   0.,
         0.,   0.,   0.,  10.;

    P << 10.,     0.,      0.,      0.,      0.,      0.,     0.,
         0.,     10.,      0.,      0.,      0.,      0.,      0.,
         0.,      0.,     10.,      0.,      0.,      0.,      0.,
         0.,      0.,      0.,     10.,      0.,      0.,      0.,
         0.,      0.,      0.,      0.,  10000.,      0.,      0.,
         0.,      0.,      0.,      0.,      0.,  10000.,      0.,
         0.,      0.,      0.,      0.,      0.,      0.,  10000.;

    // Construct the filter
    Eigen::VectorXd x0(n);
    std::vector<double> initMeasure = convert_bbox_to_z(bbox);
    x0 << initMeasure[0], initMeasure[1], initMeasure[2], initMeasure[3], 0, 0, 0;

    this->kf = KFTrack(A, C, Q, R, P, x0);
}

void KalmanBoxTracker::update(std::vector<double> bbox)
{
    this->timeSinceUpdate = 0;
    this->history.clear();
    this->hits += 1;
    this->hitStreak += 1;
    std::vector<double> measure = convert_bbox_to_z(bbox);
    Eigen::VectorXd y(this->dimZ);
    y << measure[0], measure[1], measure[2], measure[3];
    this->kf.update(y);
}

std::vector<double> KalmanBoxTracker::predict()
{
    Eigen::VectorXd state = this->kf.predict();

    if(this->timeSinceUpdate > 0){
        this->hitStreak = 0;
    }
    this->timeSinceUpdate += 1;

    std::vector<double> stateVector;
    for(int i = 0; i < this->dimX; i ++){
        stateVector.push_back(state.coeff(i));
    }
    std::vector<double> predBBox = convert_x_to_bbox(stateVector);
    this->history.push_back(predBBox);
    return predBBox;
}

std::vector<double> KalmanBoxTracker::getState()
{
    Eigen::VectorXd state = this->kf.state();
    std::vector<double> stateVector;
    for(int i = 0; i < this->dimX; i ++){
        stateVector.push_back(state.coeff(i));
    }
    std::vector<double> updatedBBox = convert_x_to_bbox(stateVector);
    return updatedBBox;
}

void KalmanBoxTracker::updateWithPredict()
{
    this->kf.updateWithPredicted();
}

/**
 * @brief IOU//
 * @param box1 (x1, y1, x2, y2)
 * @param box2 (x1, y1, x2, y2)
 * @return IOU
 */
static double IOU(const BBox & box1, const BBox & box2)
{
    double minX, minY, maxX, maxY;
    double height1 = box1[3] - box1[1] + 1.0;
    double width1 = box1[2] - box1[0] + 1.0;
    double height2 = box2[3] - box2[1] + 1.0;
    double width2 = box2[2] - box2[0] + 1.0;

    std::vector<double> XValues = {box1[0], box1[2], box2[0], box2[2]};
    std::vector<double> YValues = {box1[1], box1[3], box2[1], box2[3]};

    std::sort(XValues.begin(), XValues.end());
    std::sort(YValues.begin(), YValues.end());

    minX = XValues.front(); maxX = XValues.back();
    minY = YValues.front(); maxY = YValues.back();

    double unionHeight = maxY - minY + 1;
    double unionWidth = maxX - minX + 1;

    double intersecHeight = height1 + height2 - unionHeight;
    intersecHeight = intersecHeight > 0.1 ? intersecHeight : 0;
    double intersecWidth = width1 + width2 - unionWidth;
    intersecWidth = intersecWidth > 0.1 ? intersecWidth : 0;

    double intersecArea = intersecHeight * intersecWidth;
    double area1 = height1 * width1;
    double area2 = height2 * width2;
    double iou = intersecArea / (area1 + area2 - intersecArea);
    return iou;
}


static void AssociateDetectionsToPredicts(const BBoxArray & dets, const BBoxArray & predicts,
                                          std::map<long, long> & predictToDetMatching)
{
    double iouThreshold = 0.3;
    predictToDetMatching.clear();

    if( predicts.empty()){
        return;
    }

    size_t numDets = dets.size();
    size_t numPredicts = predicts.size();
    size_t maxSize = std::max(numDets, numPredicts);

    // calcuate IOU
    dlib::matrix<int> IOUMatrix(maxSize, maxSize);

    for(size_t detIdx = 0; detIdx < maxSize; detIdx ++){
        for(size_t predIdx = 0; predIdx < maxSize; predIdx ++){
            if( detIdx < numDets && predIdx < numPredicts) {
                double iou = IOU(dets[detIdx], predicts[predIdx]);
                IOUMatrix(detIdx, predIdx) = (int) (iou * 10000);
            }else{
                IOUMatrix(detIdx, predIdx) = 0;
            }
        }
    }

    std::vector<long> detToPredict = dlib::max_cost_assignment(IOUMatrix);

    for(long i = 0; i < (long)numDets; i ++){
        long predIdx = detToPredict[i];
        if(predIdx >= (long)numPredicts) continue;

        long detIdx = i;
        if((double)IOUMatrix(detIdx, predIdx) / 10000 >= iouThreshold){
            predictToDetMatching.insert(std::pair<size_t, size_t>(predIdx, detIdx));
        }
    }
}

#if 0
static BBox FuseBBox(const BBox & box1, const BBox & box2, double alpha)
{
    BBox fusedBBox;
    for(int i = 0; i < 4; i ++){
        fusedBBox.push_back(alpha * box1[i] + (1.0 - alpha) * box2[i]);
    }
    return fusedBBox;
}
#endif

void LpTracker::getPredicts(BBoxArray & predicts)
{
    predicts.clear();
    auto trackIter = this->KFBoxTrackers.begin();

    while( trackIter != this->KFBoxTrackers.end()){
        BBox p = trackIter->predict();
        bool pValid = ! std::isnan(p[0]) && ! std::isnan(p[1])
                      && ! std::isnan(p[2]) && ! std::isnan(p[3]);

        if (! pValid){
            trackIter = this->KFBoxTrackers.erase(trackIter);
            continue;
        }

        predicts.push_back(p);
        trackIter ++;
    }
}

void LpTracker::updateMatchingSets(const std::map<long, long> & predToDetMatching)
{
    this->matchedPredicts.clear();
    this->matchedDets.clear();
    auto matchIter = predToDetMatching.begin();

    while( matchIter != predToDetMatching.end()){
        long predIdx = matchIter->first;
        long detIdx = matchIter->second;
        this->matchedPredicts.insert(predIdx);
        this->matchedDets.insert(detIdx);

        detToObject.insert(std::pair<long, long>(detIdx, predIdx));

        matchIter ++;
    }
}

void LpTracker::updateMatchedTrackers(const std::map<long, long> & predictToDetMatching,
                           const BBoxArray & predicts, const BBoxArray & dets)
{
    // Update matched trackers.
    auto iter = predictToDetMatching.begin();
    while( iter != predictToDetMatching.end()){
        long detIdx = (*iter).second;
        long predIdx = (*iter).first;
        BBox detBBox = dets[detIdx];
        this->KFBoxTrackers[predIdx].update(detBBox);
        iter ++;
    }
}


void LpTracker::updateUnmatchedTrackers()
{
    for(long trackIdx = 0; trackIdx < (long)(this->KFBoxTrackers.size()); trackIdx ++) {
        if (matchedPredicts.find(trackIdx) == matchedPredicts.end()) {
            KalmanBoxTracker & boxTracker = this->KFBoxTrackers[trackIdx];
            boxTracker.updateWithPredict();
        }
    }
}

void LpTracker::addNewTrackers(const BBoxArray & dets)
{
    for(long detIdx = 0; detIdx < (long)dets.size(); detIdx ++){
        if( matchedDets.find(detIdx) != matchedDets.end()){
            continue;
        }
        detToObject.insert(std::pair<int, int>(detIdx, this->count));
        KalmanBoxTracker boxTracker(this->count,dets[detIdx]);
        this->count +=1;

        this->KFBoxTrackers.push_back(boxTracker);
    }
}


void LpTracker::removeInvalidTrackers(int imageWidth, int imageHeight)
{
    auto trackIter = this->KFBoxTrackers.begin();

    while(trackIter != this->KFBoxTrackers.end()){
        bool needToErase = (*trackIter).getTimeSinceUpdate() > this->maxAge;

        if( ! needToErase) {
            BBox bbox = (*trackIter).getState();

            if ((bbox[2] - bbox[0]) * (bbox[3] - bbox[1]) < 20 * 20) needToErase = true;
            else if (bbox[0] < 1.0 || bbox[1] < 1.0) needToErase = true;
            else if (bbox[2] > imageWidth - 1 || bbox[3] > imageHeight - 1) needToErase = true;
        }

        if(needToErase) {
            trackIter = this->KFBoxTrackers.erase(trackIter);
            continue;
        }

        trackIter ++;
    }
}

void LpTracker::getCurrentState(BBoxArrayWithId & results)
{
    results.clear();
    auto trackIter = this->KFBoxTrackers.begin();
    while(trackIter != this->KFBoxTrackers.end()){
        BBox currentBox = (*trackIter).getState();
        int id = (*trackIter).getId();
        BBoxWithId currentBoxWithId;
        for(int i = 0; i < 4; i ++) currentBoxWithId.push_back((int)(currentBox[i]));
        currentBoxWithId.push_back(id);
        results.push_back(currentBoxWithId);
        trackIter ++;
    }
}

BBoxArrayWithId LpTracker::update(const BBoxArray & dets, int width, int height)
{
    // predict, will remove trackers which produce NaN values.
    BBoxArray predicts;
    getPredicts(predicts);

    detToObject.clear();
    // associate detections to trackers.
    std::map<long, long>  predictToDetMatching;
    if( ! dets.empty()  && ! this->KFBoxTrackers.empty()){
        AssociateDetectionsToPredicts(dets, predicts, predictToDetMatching);
    }

    updateMatchingSets(predictToDetMatching);

    updateMatchedTrackers(predictToDetMatching, predicts, dets);

    // Update unmatched trackers use predict directly
    updateUnmatchedTrackers();

    // Add new trackers for un-matched detections.
    addNewTrackers(dets);

    // remove trackers not valid.
    removeInvalidTrackers(width, height);

    // return the current states (converted to bbox) of trackers.
    BBoxArrayWithId results;
    getCurrentState(results);

    return results;
}

cv::Rect2d convertBBoxToOcvBBox(const BBox & bbox)
{
    cv::Rect2d rect;
    double width = bbox[2] - bbox[0] + 1.0;
    double height = bbox[3] - bbox[1] + 1.0;
    rect.x = bbox[0];
    rect.y = bbox[1];
    rect.width = width;
    rect.height = height;
    return rect;
}

BBox convertOcvBBoxToBBox(const cv::Rect2d & rect)
{
    BBox bbox;
    double x2 = rect.x + rect.width - 1.0;
    double y2 = rect.y + rect.height - 1.0;
    bbox.push_back(rect.x);
    bbox.push_back(rect.y);
    bbox.push_back(x2);
    bbox.push_back(y2);
    return bbox;
}


void LpTracker::recoverID(int currentID, int targetID)
{
    auto trackIter = this->KFBoxTrackers.begin();
    while(trackIter != this->KFBoxTrackers.end()){
        int id = (*trackIter).getId();
        if (id == currentID) {
            (*trackIter).changeID(targetID);
            this->count -= 1;
            break;
        }
        trackIter ++;
    }
}

bool LpTracker::findObjectByID(int queryID)
{
    auto trackIter = this->KFBoxTrackers.begin();
    while(trackIter != this->KFBoxTrackers.end()){
        int id = (*trackIter).getId();
        if (id == queryID)
        {
            return true;
        }
        trackIter ++;
    }
    return false;
}

//-------------------------------------------------------------------------
//
//  parser
//
//-------------------------------------------------------------------------
static void parseTrackResult(const BBoxArrayWithId & bboxArrayWithId, std::vector<ObjectData> & trackResult)
{
    trackResult.clear();
    for(auto & boxWidthId : bboxArrayWithId) {
        int x1 = boxWidthId[0];
        int y1 = boxWidthId[1];
        int x2 = boxWidthId[2];
        int y2 = boxWidthId[3];
        int id = boxWidthId[4];
        int width = x2 - x1 + 1;
        int height = y2 - y1 + 1;
        ObjectData trackRes;
        trackRes.id = id;
        trackRes.rect.x = x1;
        trackRes.rect.y = y1;
        trackRes.rect.width = width;
        trackRes.rect.height = height;
        trackResult.push_back(trackRes);
    }
}


