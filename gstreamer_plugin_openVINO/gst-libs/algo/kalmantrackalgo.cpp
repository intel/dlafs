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
#include "kalmantrackalgo.h"
#include <interface/videodefs.h>


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
     KalmanTrackAlgo *tackAlgo = static_cast<KalmanTrackAlgo*> (algoData->algoBase);
     BBoxArrayWithId bboxArrayWithId;
     MathUtils utils;

       // verify the input object data
       tackAlgo->verify_detection_result(algoData->mObjectVec);
    
       // If not receive for 3 frame, clear all tracked objects
       if(tackAlgo->mFrameIndex - tackAlgo->mFrameIndexLast > 3) {
                tackAlgo->mTrackObjVec.clear();
       }
       tackAlgo->mFrameIndexLast = tackAlgo->mFrameIndex;
    
        // get object box 
        auto vecDetectRt = algoData->mObjectVec;
        BBoxArray boxes;
        for(auto & objectData : vecDetectRt){
                // convert it from orignal size base to tracking image base
                cv::Rect2d cvbox = utils.convert_rect(objectData.rect, 
                                     tackAlgo->mImageProcessorInVideoWidth,
                                     tackAlgo->mImageProcessorInVideoHeight,
                                     tackAlgo->mInputWidth, tackAlgo->mInputHeight);
                boxes.push_back(convertOcvBBoxToBBox(cvbox));
        }
    
      // track
      std::vector<ObjectData> trackResult;
       bboxArrayWithId = tackAlgo->kalmanTracker->update(boxes, tackAlgo->mImageProcessorInVideoWidth, 
            tackAlgo->mImageProcessorInVideoHeight);
       // trackResult is the track result, but not used for LP detection
       const std::map<long, long> & boxIDToObjectID = tackAlgo->kalmanTracker->getBoxToObjectMapping();
       parseTrackResult(bboxArrayWithId, trackResult);
       for(guint i=0; i<trackResult.size();i++) {
               trackResult[i].rect = utils.convert_rect(trackResult[i].rect,
                                     tackAlgo->mInputWidth, tackAlgo->mInputHeight,
                                     tackAlgo->mImageProcessorInVideoWidth,
                                     tackAlgo->mImageProcessorInVideoHeight);
        }
    
       // loop each object and detect LP
       int lpIdx=0;
       for(guint i=0; i<algoData->mObjectVec.size();i++) {
             ObjectData &objectData = algoData->mObjectVec[i];
             objectData.flags = 0;
    
             auto iter = boxIDToObjectID.find(i);
             if(iter == boxIDToObjectID.end()){
                    continue;
              }
                lpIdx ++;
                objectData.flags = 1;
                 tackAlgo->try_add_new_one_object(objectData, algoData->mFrameId);
        }
       //trackLpAlgo->print_objects(algoData->mObjectVec);
       tackAlgo->remove_invalid_object(algoData->mObjectVec);
    
         // update mTrackObjVec
        tackAlgo->verify_tracked_object();
        tackAlgo->update_track_object(algoData->mObjectVec);
        tackAlgo->mLastObjectTrackRes = trackResult;

}

KalmanTrackAlgo::KalmanTrackAlgo():CvdlAlgoBase(post_tracklp_process, CVDL_TYPE_CV)
{
    mInputWidth = TRACKING_LP_INPUT_W;
    mInputHeight = TRACKING_LP_INPUT_H;

    mSvmModelStr = std::string(CVDL_MODEL_DIR_DEFAULT"/svm_model.xml");
    kalmanTracker = new KalmanTracker(KFMaxAge);
}

KalmanTrackAlgo::~KalmanTrackAlgo()
{
    delete kalmanTracker;
    g_print("KalmanTrackAlgo: image process %d frames, image preprocess fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost);
}

// set data caps
//   1. pass orignal video caps to it, which is the input data of image processor
//   2. create ocl caps for OCL processing the orignal video image to be the Gray format, which is the input of object tracking
void KalmanTrackAlgo::set_data_caps(GstCaps *incaps)
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    // set OCL surface: width and height and format
    //   track will use BGR data format
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
void KalmanTrackAlgo::verify_detection_result(std::vector<ObjectData> &objectVec)
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

        vecObjectCp[i].flags  = 0;
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

void KalmanTrackAlgo::try_add_new_one_object(ObjectData &objectData, guint64 frameId)
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
void KalmanTrackAlgo::verify_tracked_object()
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

bool KalmanTrackAlgo::is_at_buttom(TrackLpObjAttribute& curObj)
{
    cv::Rect rt = curObj.getLastPos();

    if (rt.height < 60 && rt.y + rt.height > mInputHeight - 5) {
        return true;
    }

    return false;
}

void KalmanTrackAlgo::update_track_object(std::vector<ObjectData> &objectVec)
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

void KalmanTrackAlgo::remove_invalid_object(std::vector<ObjectData> &objectVec)
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


