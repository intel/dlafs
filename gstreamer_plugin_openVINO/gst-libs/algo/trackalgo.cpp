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
#include "mathutils.h"
#include "trackalgo.h"
#include <interface/videodefs.h>

using namespace cv;

#define CHECK_ROI(rt, W, H) \
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
#define TRACK_FRAME_NUM 3

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
    gint64 start, stop;

    GST_LOG("\ntrack_algo_func - new an algoData = %p\n", algoData);

    if(!trackAlgo->mNext) {
        GST_WARNING("TrackAlgo: the next algo is NULL, return!");
        return;
    }

    if(!trackAlgo->mInQueue.get(*algoData)) {
        GST_WARNING("TrackAlgo:InQueue is empty!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    if(algoData->mGstBuffer==NULL) {
        GST_WARNING("Invalid buffer!!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    start = g_get_monotonic_time();

    // bind algoTask into algoData, so that can be used when sync callback
    algoData->algoBase = static_cast<CvdlAlgoBase *>(trackAlgo);
    trackAlgo->mInferCnt=1;

    GST_LOG("%s() - trackAlgo = %p, algoData->mFrameId = %ld\n",
            __func__, trackAlgo, algoData->mFrameId);
    GST_LOG("%s() - get one buffer, GstBuffer = %p, refcout = %d, queueSize = %d, algoData = %p, algoBase = %p\n",
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
        trackAlgo->mInferCnt=0;
        return;
    }
    algoData->mGstBufferOcl = ocl_buf;

    OclMemory *ocl_mem = NULL;
    ocl_mem = ocl_memory_acquire (ocl_buf);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        trackAlgo->mInferCnt=0;
        return;
    }

    //test
    //trackAlgo->save_buffer(ocl_mem->frame.getMat(0).ptr(), trackAlgo->mInputWidth,
    //                   trackAlgo->mInputHeight,1,algoData->mFrameId, 1, "track");

    trackAlgo->verify_detection_result(algoData->mObjectVec);

    // Tracking every object, and get predicts.
    #if 0
    trackAlgo->mImageProcessor.ocl_lock();
    trackAlgo->track_objects(algoData);
    trackAlgo->mImageProcessor.ocl_unlock();
    #else
    trackAlgo->track_objects_fast(algoData);
    #endif
    trackAlgo->update_track_object(algoData->mObjectVec);

    stop = g_get_monotonic_time();
    trackAlgo->mImageProcCost += (stop - start);

    // push data if possible
    trackAlgo->push_track_object(algoData);
    trackAlgo->mInferCnt=0;
    trackAlgo->mInferCntTotal++;
    trackAlgo->mFrameDoneNum++;
}


TrackAlgo::TrackAlgo():CvdlAlgoBase(track_algo_func, this, NULL)
{
    mInputWidth = TRACKING_INPUT_W;
    mInputHeight = TRACKING_INPUT_H;

    mPreFrame = NULL;
    mOclCaps = NULL;
    mInCaps = NULL;
}

TrackAlgo::~TrackAlgo()
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    g_print("TrackAlgo: image process %d frames, image preprocess fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost);
}

cv::UMat& TrackAlgo::get_umat(GstBuffer *buffer)
{
    OclMemory *ocl_mem = NULL;
    static cv::UMat empty_umat = cv::UMat();
    // On Intel GPU, UMat::getMat will map device memory into host space
    // for CPU drawing function to work, before display or further access
    // this UMat using OpenCL, we need ensure its unmapped by make sure
    // the Mat is destructed.
    ocl_mem = ocl_memory_acquire (buffer);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        return empty_umat;
    }
    return ocl_mem->frame;
}

cv::Mat TrackAlgo::get_mat(GstBuffer *buffer)
{
    OclMemory *ocl_mem = NULL;
    static cv::Mat empty_mat = cv::Mat();
    ocl_mem = ocl_memory_acquire (buffer);
    if(ocl_mem==NULL){
        GST_WARNING("Failed get ocl_mem after image process!");
        return empty_mat;
    }
    cv::Mat mat = ocl_mem->frame.getMat(0);
    return mat;
}

// set data caps
//   1. pass orignal video caps to it, which is the input data of image processor
//   2. create ocl caps for OCL processing the orignal video image to be the Gray format, which is the input of object tracking
void TrackAlgo::set_data_caps(GstCaps *incaps)
{
    if(mInCaps)
        gst_caps_unref(mInCaps);
    mInCaps = gst_caps_copy(incaps);

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

    cv::goodFeaturesToTrack(gray, vecFeatPt, MAX_COUNT, 0.3, 7, cv::Mat(), 7, false, 0.03);
    if(vecFeatPt.size() == 0){
        return vecFeatPt;
    }
    cv::cornerSubPix(gray, vecFeatPt, subPixWinSize, cv::Size(-1, -1), termcrit);
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
bool TrackAlgo::track_one_object(cv::Mat& curFrame, TrackObjAttribute& curObj, cv::Rect& outRoi)
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

        GST_LOG("%s(): can't find feature pt\n",__func__);

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
        cv::Mat preFrame = get_mat(mPreFrame);
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

        SET_SATURATE(offx, -50, 50);
        SET_SATURATE(offy, 1, 100);
    }

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

    return true;
}

void TrackAlgo::figure_out_trajectory_points(
    ObjectData &objectVec, TrackObjAttribute& curObj)
{
    std::vector<cv::Rect> &rectVec = curObj.vecPos;
    cv::Rect rect;
    objectVec.trajectoryPoints.clear();
    MathUtils utils;
    for(unsigned int i=0; i<rectVec.size(); i++) {
        VideoPoint point;
        rect = utils.convert_rect(
            rectVec[i],mInputWidth, mInputHeight,
            mImageProcessorInVideoWidth,
            mImageProcessorInVideoHeight);
        point.x = rect.x + rect.width/2;
        point.y = rect.y + rect.height/2;
        objectVec.trajectoryPoints.push_back(point);
    }
    return ;
}

cv::Rect TrackAlgo::compare_detect_predict(
    std::vector<ObjectData>& objectVec, TrackObjAttribute& curObj, 
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
            // put the trajectory points to it
            figure_out_trajectory_points(objectVec[i], curObj);
            VideoPoint point = {objRectOrig.x+objRectOrig.width/2,
                                objRectOrig.y+objRectOrig.height/2};
            objectVec[i].trajectoryPoints.push_back(point);
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

    // The ROI is the car's face part
    // another method is to use LP to detect car's face
    curRect.y = curRect.y + curRect.height/2;
    curRect.height = curRect.height/2;
    roiRect = utils.convert_rect(curRect, mInputWidth, mInputHeight,
                       mImageProcessorInVideoWidth, mImageProcessorInVideoHeight);
}

// Add a new rect to tracking obj, check whether crop roi in the src image.
//    curRT is based on trackinge size
void TrackAlgo::add_track_obj(CvdlAlgoData* &algoData,
    cv::Rect curRt, TrackObjAttribute& curObj, bool bDetect)
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
    }
}

void TrackAlgo::add_new_one_object(ObjectData &objectData, guint64 frameId)
{
    TrackObjAttribute obj;
    cv::Rect objRect = objectData.rect;
    MathUtils utils;

    obj.objId = mCurObjId;
    obj.score = 0.0;
    obj.detectedNum = 0;
    obj.fliped = false;
    obj.startFrameId = frameId;
    obj.curFrameId = obj.startFrameId;

    obj.notDetectNum = 0;
    obj.bBottom = false;

    objectData.flags |= FLAGS_TRACKED_DATA_IS_SET;

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
    
void TrackAlgo::track_objects_fast(CvdlAlgoData* &algoData)
{
    // Rect is based on orignal video frame
    std::vector<ObjectData>& objectVec = algoData->mObjectVec;

    // clear this flag
    for(size_t i=0; i<objectVec.size(); i++) {
        objectVec[i].flags &= ~(FLAGS_TRACKED_DATA_IS_SET|FLAGS_TRACKED_DATA_IS_PASS);
        objectVec[i].id = -1;
    }

    // firstly, match every existed trackObject by detection result
    for (size_t i = 0; i < mTrackObjVec.size(); i++) {
        TrackObjAttribute& curObj = mTrackObjVec[i];

        // No tracking, waiting for report.
        if (curObj.notDetectNum >= TRACK_MAX_NUM) {
            continue;
        }

        // Tracking current object
        cv::Rect outRoi;
        cv::Mat empty_mat = cv::Mat();
        if (track_one_object(empty_mat, curObj, outRoi)) {
            CHECK_ROI(outRoi, mInputWidth, mInputHeight);

            // Compare with real-time detect result.
            bool bDetect = false;
            cv::Rect curRt = compare_detect_predict(objectVec, curObj, outRoi, bDetect);
            CHECK_ROI(curRt, mInputWidth, mInputHeight);

            add_track_obj(algoData, curRt, curObj, bDetect);
        }
    }

    /* Check whether vecDetectRt.size == null?, if !=null, add new object */
    add_new_objects(objectVec, algoData->mFrameId);

    if(mPreFrame)
        gst_buffer_unref(mPreFrame);
    // Don't use preFrame
    mPreFrame = NULL;
}
void TrackAlgo::track_objects(CvdlAlgoData* &algoData)
{
    // Rect is based on orignal video frame
    std::vector<ObjectData>& objectVec = algoData->mObjectVec;

    // clear this flag
    for(size_t i=0; i<objectVec.size(); i++) {
        objectVec[i].flags &= ~(FLAGS_TRACKED_DATA_IS_SET|FLAGS_TRACKED_DATA_IS_PASS);
        objectVec[i].id = -1;
    }

    //cv::UMat &preFrame = mPreFrame ? get_umat(mPreFrame) : cv::UMat();
    cv::Mat curFrame = get_mat(algoData->mGstBufferOcl);
    cv::UMat &curUFrame = get_umat(algoData->mGstBufferOcl);
    mCurFeaturePt = calc_feature_points(curUFrame);

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
            CHECK_ROI(outRoi, mInputWidth, mInputHeight);

            // Compare with real-time detect result.
            bool bDetect = false;
            cv::Rect curRt = compare_detect_predict(objectVec, curObj, outRoi, bDetect);
            CHECK_ROI(curRt, mInputWidth, mInputHeight);

            add_track_obj(algoData, curRt, curObj, bDetect);
        }
    }

    /* Check whether vecDetectRt.size == null?, if !=null, add new object */
    add_new_objects(objectVec, algoData->mFrameId);

    if(mPreFrame)
        gst_buffer_unref(mPreFrame);
    // DOn't use preFrame
    mPreFrame = NULL;
    //mPreFrame = algoData->mGstBufferOcl;
    mPreFeaturePt = mCurFeaturePt;
}


bool TrackAlgo::is_at_buttom(TrackObjAttribute& curObj)
{
    cv::Rect rt = curObj.getLastPos();

    if (rt.height < 60 && rt.y + rt.height > mInputHeight - 5) {
        return true;
    }

    return false;
}

void TrackAlgo::update_track_object(std::vector<ObjectData> &objectVec)
{
    std::vector<TrackObjAttribute>::iterator it;
    float score = 0.0;

    for (it = mTrackObjVec.begin(); it != mTrackObjVec.end();) {
        // find the connected objectData
        for(unsigned int i=0; i<objectVec.size(); i++){
            ObjectData& objectData = objectVec[i];
            if(objectData.id == (*it).objId) {
                score = objectData.figure_score(mImageProcessorInVideoWidth,
                    mImageProcessorInVideoHeight);
                if(score>1.0) {
                    (*it).detectedNum ++;
                }
                if((score == 0.0) && ((*it).score > 1.0) &&
                    (*it).fliped==false && ((*it).detectedNum >= TRACK_FRAME_NUM)) {
                    objectData.flags |= FLAGS_TRACKED_DATA_IS_PASS;
                    (*it).fliped = true;
                    (*it).detectedNum = 0;
                    objectData.score = (*it).score;
                }
                (*it).score = score;
                break;
            }
        }

        bool bButtom = is_at_buttom(*it);
        if (it->notDetectNum >= TRACK_MAX_NUM || bButtom) {
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

    // Must release intermedia buffer
    gst_buffer_unref(algoData->mGstBufferOcl);

    // Not tracking data need to pass to next algo component
    if(objectVec.size()==0) {
        GST_LOG("track_algo_func - unref GstBuffer = %p(%d)\n",
            algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));
        gst_buffer_unref(algoData->mGstBuffer);
        delete algoData;
        return;
    }

    GST_LOG("track_algo_func - pass down GstBuffer = %p(%d)\n",
         algoData->mGstBuffer, GST_MINI_OBJECT_REFCOUNT(algoData->mGstBuffer));

    //debug
    for(size_t i=0; i< objectVec.size(); i++) {
        GST_LOG("%d - track_output-%ld-%ld: prob = %f, label = %s, rect=(%d,%d)-(%dx%d), score = %f\n",
            mFrameDoneNum, algoData->mFrameId, i, objectVec[i].prob, objectVec[i].label.c_str(),
            objectVec[i].rect.x, objectVec[i].rect.y,
            objectVec[i].rect.width, objectVec[i].rect.height, objectVec[i].score);
    }
    mNext->mInQueue.put(*algoData);
}

