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
#include <ocl/oclmemory.h>
//#include <ocl/crcmeta.h>
//#include <ocl/metadata.h>
#include "mathutils.h"
#include "trackalgo.h"

using namespace cv;
//using namespace HDDLStreamFilter;


#define CKECK_ROI(rt, W, H) \
        if(rt.x < 0 || rt.y < 0 || rt.x + rt.width > W || rt.y + rt.height > H ||   \
           rt.width < 0 || rt.height < 0){  \
            GST_ERROR("check roi = [%d,%d,%d,%d]\n", rt.x, rt.y, rt.width, rt.height); \
            std::exit(-1);    \
        }


#define TRACKING_INPUT_W 512
#define TRACKING_INPUT_H 512

/**
 * If not detect number >= TRACK_MAX_NUM, tracking will stop.
 */
#define TRACK_MAX_NUM 12

#define FLAGS_TRACKED_DATA_IS_SET   0x1
#define FLAGS_TRACKED_DATA_IS_PASS  0x2

using namespace HDDLStreamFilter;

// main function for track algorithm
//   If there are valide detection objects, track each one, if not, return
//   1. CSC from NV12 to BGR, resize for 1920x1080 to 512x512
//   2. track each one object
//   3. put the result into next InQueue
static void track_algo_func(gpointer userData)
{
    TrackAlgo *trackAlgo = static_cast<TrackAlgo*> (userData);
    CvdlAlgoData *algoData = new CvdlAlgoData;
    //GstFlowReturn ret;

    g_print("\ntrack_algo_func - new an algoData = %p\n", algoData);

    if(!trackAlgo->mNext) {
        GST_WARNING("TrackAlgo: the next algo is NULL, return!");
        return;
    }

    if(!trackAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("TrackAlgo:InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(trackAlgo);

    g_print("%s() - trackAlgo = %p, algoData->mFrameId = %ld\n", __func__, trackAlgo, algoData->mFrameId);
    g_print("%s() - get one buffer, GstBuffer = %p, refcout = %d, queueSize = %d, algoData = %p, algoBase = %p\n",
        __func__, algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT (algoData->mGstBuffer),
        trackAlgo->mInQueue.size(), algoData, algoData->algoBase);


    // get input data and process it here, put the result into algoData
    // NV12-->BGR
    GstBuffer *ocl_buf = NULL;
    VideoRect crop = {0,0, (unsigned int)trackAlgo->mImageProcessorInVideoWidth,
                           (unsigned int)trackAlgo->mImageProcessorInVideoHeight};
    trackAlgo->mImageProcessor.process_image(algoData->mGstBuffer,NULL, &ocl_buf, &crop);
    if(ocl_buf==NULL) {
        GST_WARNING("Failed to do image process!");
        return;
    }
    algoData->mGstBufferOcl = ocl_buf;

    OclMemory *ocl_mem = NULL;
    ocl_mem = ocl_memory_acquire (ocl_buf);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        return;
    }

    //trackAlgo->push_track_object(algoData);//test

    // do object track
    /**
       * HDDL detect result x, y, width or height may be < 0
       */
    trackAlgo->verify_detection_result(algoData->mObjectVec);

    // Tracking every object, and get predicts.
    trackAlgo->track_objects(algoData);

    //------------test---remove this code -----------------
    // update object data
    //trackAlgo->update_track_object(algoData->mObjectVec);

    // push data if possible
    trackAlgo->push_track_object(algoData);
}


TrackAlgo::TrackAlgo():CvdlAlgoBase(track_algo_func, this, NULL)
{
    mInputWidth = TRACKING_INPUT_W;
    mInputHeight = TRACKING_INPUT_H;
    //mImageProcessor.set_ocl_kernel_name(CRC_FORMAT_GRAY);

    mPreFrame = NULL;
    mOclCaps = NULL;
    mInCaps = NULL;
}

TrackAlgo::~TrackAlgo()
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
}

cv::UMat& TrackAlgo::get_umat(GstBuffer *buffer)
{
    OclMemory *ocl_mem = NULL;
    static cv::UMat empty_umat = cv::UMat();
    ocl_mem = ocl_memory_acquire (buffer);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        return empty_umat;
    }
    return ocl_mem->frame;
}

// set data caps
//   1. pass orignal video caps to it, which is the input data of image processor
//   2. create ocl caps for OCL processing the orignal video image to be the Gray format, which is the input of object tracking
void TrackAlgo::set_data_caps(GstCaps *incaps)
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

    //if(mOclCaps)
    //    gst_caps_unref(mOclCaps);

    // set OCL surface: width and height and format
    //   Object track will use gray data format
    //int oclSize = mInputWidth * mInputHeight * 1;
    mOclCaps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "GRAY8", NULL);
    gst_caps_set_simple (mOclCaps, "width", G_TYPE_INT, mInputWidth, "height",
      G_TYPE_INT, mInputHeight, NULL);

    mImageProcessor.ocl_init(incaps, mOclCaps, IMG_PROC_TYPE_OCL_CRC, CRC_FORMAT_GRAY);
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
void TrackAlgo::verify_detection_result(std::vector<ObjectData> &objectVec)
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

std::vector<cv::Point2f> TrackAlgo::calc_feature_points(cv::UMat &gray)
{
    /* Optical flow parameter */
    cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03);
    const int MAX_COUNT = 100;
    cv::Size subPixWinSize(10, 10), winSize(15, 15);

    std::vector<cv::Point2f> vecFeatPt;

    //std::cout << "goodFeaturesToTrack begin....." << std::endl;
    cv::goodFeaturesToTrack(gray, vecFeatPt, MAX_COUNT, 0.3, 7, cv::Mat(), 7, false, 0.03);
    // std::cout << "goodFeaturesToTrack end ..... fea point num = " << vecFeatPt.size() << std::endl;

    if(vecFeatPt.size() == 0){
        return vecFeatPt;
    }
    cv::cornerSubPix(gray, vecFeatPt, subPixWinSize, cv::Size(-1, -1), termcrit);
    //std::cout << "cornerSubPix end .... fea point num = " << vecFeatPt.size() << std::endl;

    return vecFeatPt;
}

std::vector<cv::Point2f> TrackAlgo::find_points_in_ROI(cv::Rect roi)
{
    float x1 = roi.x;
    float y1 = roi.y;
    float x2 = roi.x + roi.width;
    float y2 = roi.y + roi.height;

    std::vector<cv::Point2f> pt;
    for (size_t i = 0; i < mPreFeaturePt.size(); i++) {
    if (mPreFeaturePt[i].x > x1 && mPreFeaturePt[i].x < x2 &&
        mPreFeaturePt[i].y > y1 && mPreFeaturePt[i].y < y2){
            pt.push_back(mPreFeaturePt[i]);
        }
    }
    return pt;
}

// Based on average shift moment estimating.If fisrt track, default = 2;
void TrackAlgo::calc_average_shift_moment(TrackObjAttribute& curObj, float& offx, float& offy)
{
    float posSz = (float) curObj.vecPos.size();
    if (curObj.vecPos.size() <= 0) {
        offx = 0;
        offy = 2.0f;
    } else {
        for (size_t i = 0; i < curObj.vecPos.size(); i++) {
            offx += (float) (curObj.vecPos[i].x + (curObj.vecPos[i].width >> 1));
            offy += (float) (curObj.vecPos[i].y + (curObj.vecPos[i].height >> 1));
        }
        offx /= posSz;
        offy /= posSz;
    }
}

// return outRoi based on the size of tracking image
bool TrackAlgo::track_one_object(cv::UMat& curFrame, TrackObjAttribute& curObj, cv::Rect& outRoi)
{
    // size is based on track image size
    cv::Rect roi = curObj.getLastPos();

    // Find good feature in the "roi".
    std::vector<cv::Point2f> roiFeature = find_points_in_ROI(roi);
    std::vector<cv::Point2f> newRoiFeature;
    float offx = 0;
    float offy = 0;

    // There is not feature point.
    if (roiFeature.size() < 1) {
        // Based on average shift moment estimating.
        // If fisrt track, default = 2;
        calc_average_shift_moment(curObj, offx, offy);

        std::cout << "can't find feature pt\n";

        // Use mean sift value
        curObj.getLastShiftValue(offx, offy);
    }else {
        // Estimate all feature points average shift moment.
        std::vector<uchar> status;
        std::vector<float> err;
        /* Optical flow parameter */
        cv::TermCriteria termcrit(
                cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03);
        cv::Size subPixWinSize(10, 10), winSize(15, 15);

        newRoiFeature.resize(roiFeature.size());
        // input frame should be gray format!!!
        cv::UMat empty_umat = cv::UMat();
        cv::UMat &preFrame = mPreFrame ? get_umat(mPreFrame) : empty_umat;
        cv::calcOpticalFlowPyrLK(preFrame, curFrame, roiFeature, newRoiFeature,
                status, err, winSize, 3, termcrit, 0, 0.001);

        // Based on optical flow points, calc shift offx, offy.
        std::vector<cv::Point2f> prePt = roiFeature;
        std::vector<cv::Point2f> nxtPt = newRoiFeature;

        for (size_t j = 0; j < prePt.size(); j++) {
            offx += nxtPt[j].x - prePt[j].x;
            offy += nxtPt[j].y - prePt[j].y;
        }

        offx /= (int) prePt.size();
        offy /= (int) prePt.size();

        if (offy > 100) {
            offy = 100;
        }
        if (offy < 1) {
            offy = 1;
        }

        if (offx > 50) {
            offx = 50;
        }
        if (offx < -50) {
            offx = -50;
        }
    }

    //CKECK_ROI(roi, RSZ_DETECT_SIZE, RSZ_DETECT_SIZE);
    outRoi.x = MAX(0, FLT2INT(roi.x + offx));
    outRoi.y = MAX(0, FLT2INT(roi.y + offy));
    outRoi.x = MIN(mInputWidth - 1, outRoi.x);
    outRoi.y = MIN(mInputHeight - 1, outRoi.y);

    int x2 = FLT2INT(roi.x + roi.width - 1 + offx);
    int y2 = FLT2INT(roi.y + roi.height - 1 + offy);
    x2 = MAX(0, x2);
    y2 = MAX(0, y2);
    x2 = MIN(mInputWidth - 1, x2);
    y2 = MIN(mInputHeight - 1, y2);

    outRoi.width = x2 - outRoi.x + 1;
    outRoi.height = y2 - outRoi.y + 1;

    outRoi.width = MAX(1, outRoi.width);
    outRoi.height = MAX(1, outRoi.height);
    //std::cout << "me roi = x,y,w,h = " << roi.x << ", " << roi.y << ", " << roi.width << ", " << roi.height << ", " << std::endl;
    //std::cout << "me outRoi= x,y,w,h = " << outRoi.x << ", " << outRoi.y << ", " << outRoi.width << ", " << outRoi.height << ", " << std::endl;
    //CKECK_ROI(outRoi, RSZ_DETECT_SIZE, RSZ_DETECT_SIZE);

    return true;
}

void TrackAlgo::figure_out_trajectory_points(ObjectData &objectVec, TrackObjAttribute& curObj)
{
    std::vector<cv::Rect> &rectVec = curObj.vecPos;
    cv::Rect rect;
    objectVec.trajectoryPoints.clear();
    MathUtils utils;
    for(unsigned int i=0; i<rectVec.size(); i++) {
        cv::Point point;
        rect = utils.convert_rect(rectVec[i],mInputWidth, mInputHeight,
                           mImageProcessorInVideoWidth, mImageProcessorInVideoHeight);
        point.x = rect.x + rect.width/2;
        point.y = rect.y + rect.height/2;
        objectVec.trajectoryPoints.push_back(point);
    }
    return ;
}

cv::Rect TrackAlgo::compare_detect_predict(std::vector<ObjectData>& objectVec, TrackObjAttribute& curObj, 
                                                   cv::Rect predictRt, bool& bDetect)
{
    cv::Rect objRect;
    MathUtils utils;

    for (size_t i = 0; i < objectVec.size(); i++) {
        // Calc 2 roi overlap ratio.
        float ol12 = 0, ol21 = 0;
        cv::Rect& objRectOrig = objectVec[i].rect;
        objRect = objectVec[i].rect;

        if(objectVec[i].flags & FLAGS_TRACKED_DATA_IS_SET)
            continue;

        // convert size to be based on tracking size
        objRect = utils.convert_rect(objRect,
                           mImageProcessorInVideoWidth, mImageProcessorInVideoHeight, 
                           mInputWidth, mInputHeight);
        utils.get_overlap_ratio(objRect, predictRt, ol12, ol21);

        if (ol12 > 0.60f || ol21 > 0.4f) {
            bDetect = true;
            // set the flags of the i th element
            objectVec[i].flags |= FLAGS_TRACKED_DATA_IS_SET;
            //objectVec[i].fetch_or(FLAGS_TRACKED_DATA_IS_SET);
            // put the trajectory points to it
            figure_out_trajectory_points(objectVec[i], curObj);
            objectVec[i].trajectoryPoints.push_back(cv::Point(objRectOrig.x+objRectOrig.width/2,
                                                           objRectOrig.y+objRectOrig.height/2));
            // combine objectVec[i] with curObj
            objectVec[i].id = curObj.objId;
            return objRect;
        }
    }

    bDetect = false;
    return predictRt;
}


/**
 * get ROI based on orignal video size
 */
void TrackAlgo::get_roi_rect(cv::Rect& roiRect, cv::Rect curRect)
{
    MathUtils utils;
    roiRect = utils.convert_rect(curRect, mInputWidth, mInputHeight,
                       mImageProcessorInVideoWidth, mImageProcessorInVideoHeight);
}

// Add a new rect to tracking obj, check whether crop roi in the src image.
//    curRT is based on trackinge size
void TrackAlgo::add_track_obj(CvdlAlgoData* &algoData, cv::Rect curRt, TrackObjAttribute& curObj, bool bDetect)
{
    // Add real-time detect or track result.
    curObj.vecPos.push_back(curRt);
    curObj.curFrameId = algoData->mFrameId;

    // Check report image
    // Is real-time detect result?
    if (!bDetect){
        curObj.notDetectNum++;
    }
    else{
        // Only report detect result.
        curObj.notDetectNum = 0;

        // Check whether to cache ROI image.
        if (curObj._vecRoiPosInSrc.size() == 1){
            // If center pass middle line, cache it.
            int centerY = curRt.height / 2 + curRt.y;

            // The center of vehicle had pass middle line video,
            // we need cache it and classify it in the further.
            if (centerY > mInputHeight / 3)
            {
                cv::Rect roiRect;
                // only get the ROI rectangle, the image buffer will be crop out in the next algo(classfication)
                // roiRect is based one orignal video size
                get_roi_rect(roiRect, curRt);

                // save ROI rectange based on the orignal image
                curObj.putImg(roiRect);
            }
        }
    }
}

void TrackAlgo::add_new_one_object(ObjectData &objectData, guint64 frameId)
{
    TrackObjAttribute obj;
    cv::Rect objRect = objectData.rect;
    MathUtils utils;

    obj.objId = mCurObjId;
    obj.startFrameId = frameId;
    obj.curFrameId = obj.startFrameId;

    obj.notDetectNum = 0;
    obj.bBottom = false;

    objectData.flags |= FLAGS_TRACKED_DATA_IS_SET;
    //objectData.flags.fetch_or(FLAGS_TRACKED_DATA_IS_SET);
    objectData.trajectoryPoints.push_back(cv::Point(objRect.x+objRect.width/2,
                                                    objRect.y+objRect.height/2));

    // convert it from orignal size base to tracking image base
    objRect = utils.convert_rect(objectData.rect, 
                                      mImageProcessorInVideoWidth, mImageProcessorInVideoHeight,
                                      mInputWidth, mInputHeight);
    obj.vecPos.push_back(objRect);

    // vec is based on the orignal video size
    CKECK_ROI(objectData.rect, mImageProcessorInVideoWidth, mImageProcessorInVideoHeight);

    cv::Rect roiRect = objectData.rect;
    //get_roi_rect(roiRect, objectData.vec);
    obj.putImg(roiRect);

    mTrackObjVec.push_back(obj);

    mCurObjId++;
}

void TrackAlgo::add_new_objects(std::vector<ObjectData>& objectVec, guint64 frameId)
{
    for (size_t i = 0; i < objectVec.size(); i++) {
        add_new_one_object(objectVec[i], frameId);
    }
}

/**
* brief@ Track one frame, maybe multiple objects, this is fast method based on movement estimate.
* param@ img : src image
* param@ vecDetectRt : real-time detect result based on 'img'
* Note@ Save tracking result to 'm_vecTrackObj'
*/
void TrackAlgo::track_objects(CvdlAlgoData* &algoData)
{
    // Rect is based on orignal video frame
    std::vector<ObjectData>& objectVec = algoData->mObjectVec;

    // clear this flag
    for(size_t i=0; i<objectVec.size(); i++) {
        objectVec[i].flags &= ~(FLAGS_TRACKED_DATA_IS_SET|FLAGS_TRACKED_DATA_IS_PASS);
        //objectVec[i].flags.fetch_and(~(FLAGS_TRACKED_DATA_IS_SET|FLAGS_TRACKED_DATA_IS_PASS));
        objectVec[i].id = -1;
    }

    //cv::UMat &preFrame = mPreFrame ? get_umat(mPreFrame) : cv::UMat();
    cv::UMat empty_umat = cv::UMat();
    cv::UMat &curFrame = algoData->mGstBufferOcl ? get_umat(algoData->mGstBufferOcl) : empty_umat;
    mCurFeaturePt = calc_feature_points(curFrame);

    // firstly, match every existed trackObject by detection result
    for (size_t i = 0; i < mTrackObjVec.size(); i++) {
        TrackObjAttribute& curObj = mTrackObjVec[i];

        // No tracking, waiting for report.
        if (curObj.notDetectNum >= TRACK_MAX_NUM) {
            continue;
        }

        // Tracking current object
        cv::Rect outRoi;
        
        if (track_one_object(curFrame, curObj, outRoi)) {
            CKECK_ROI(outRoi, mInputWidth, mInputHeight);

            // Compare with real-time detect result.
            bool bDetect = false;
            cv::Rect curRt = compare_detect_predict(objectVec, curObj, outRoi, bDetect);
            CKECK_ROI(curRt, mInputWidth, mInputHeight);

            add_track_obj(algoData, curRt, curObj, bDetect);
        }
    }

    /* Check whether vecDetectRt.size == null?, if !=null, add new object */
    add_new_objects(objectVec, algoData->mFrameId);

    if(mPreFrame)
        gst_buffer_unref(mPreFrame);
    mPreFrame = algoData->mGstBufferOcl;
    mPreFeaturePt = mCurFeaturePt;
}


bool TrackAlgo::is_at_buttom(TrackObjAttribute& curObj)
{
    cv::Rect rt = curObj.getLastPos();

    //if (rt.height < 200 && rt.y + rt.height > RSZ_DETECT_SIZE - 10) {
    if (rt.height < 60 && rt.y + rt.height > mInputHeight - 5) {
        return true;
    }

    return false;
}

// Report track result to pipeline for NVR show.
void TrackAlgo::update_track_object(std::vector<ObjectData> &objectVec)
{
    // Report to pipeline thread.
    // draw results and put into display buffer

    // On Intel GPU, UMat::getMat will map device memory into host space
    // for CPU drawing function to work, before display or further access
    // this UMat using OpenCL, we need ensure its unmapped by make sure
    // the Mat is destructed.

    std::vector<TrackObjAttribute>::iterator it;

    for (it = mTrackObjVec.begin(); it != mTrackObjVec.end();) {
        bool bButtom = is_at_buttom(*it);
        if (it->notDetectNum >= TRACK_MAX_NUM || bButtom) {
            // find the connected objectData
            for(unsigned int i=0; i<objectVec.size(); i++){
                ObjectData& objectData = objectVec[i];
                if(objectData.id == (*it).objId) {
                    objectData.flags |= FLAGS_TRACKED_DATA_IS_PASS;
                    //objectData.flags.fetch_and(~FLAGS_TRACKED_DATA_IS_PASS);
                    break;
                }
            }
            it = mTrackObjVec.erase(it);    //remove item.
            continue;
        } else {
            ++it;
        }
    }

    // remove the objectData which has not been set PASS flag
    std::vector<ObjectData>::iterator obj;
    for (obj = objectVec.begin(); obj != objectVec.end();) {
        if(!((*obj).flags & FLAGS_TRACKED_DATA_IS_PASS)) {
            // remove  it
            obj = objectVec.erase(obj);
        } else {
            ++obj;
        }
    }

}


void TrackAlgo::push_track_object(CvdlAlgoData* &algoData)
{
    std::vector<ObjectData> &objectVec = algoData->mObjectVec;

    // Not tracking data need to pass to next algo component
    if(objectVec.size()==0) {
        g_print("track_algo_func - unref GstBuffer = %p(%d)\n",
            algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
        delete algoData;
        return;
    } 
    g_print("track_algo_func - pass down GstBuffer = %p(%d)\n",
         algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
    mNext->mInQueue.put(*algoData);
}

