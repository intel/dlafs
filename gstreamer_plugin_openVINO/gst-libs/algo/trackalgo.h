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

#ifndef __TRACK_ALGO_H__
#define __TRACK_ALGO_H__

#include <vector>
#include "algobase.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include <gst/gstpad.h>
#include <opencv2/opencv.hpp>

#define SET_SATURATE(X, L, H) \
        if(X<L) X=L; \
        else if(X>H) X=H;


class TrackObjAttribute;

class TrackAlgo : public CvdlAlgoBase 
{
public:
    TrackAlgo();
    virtual ~TrackAlgo();
    virtual void set_data_caps(GstCaps *incaps);

    void verify_detection_result(std::vector<ObjectData> &objectVec);
    void track_objects(CvdlAlgoData* &algoData);
    void track_objects_fast(CvdlAlgoData* &algoData);
    void push_track_object(CvdlAlgoData* &algoData);
    void update_track_object(std::vector<ObjectData> &objectVec);

    ImageProcessor mImageProcessor;
    GstCaps *mInCaps;  /* Caps for orignal input video*/
    GstCaps *mOclCaps; /* Caps for output surface of OCL, which has been CRCed, and as the input of detection algo */

    int mImageProcessorInVideoWidth;
    int mImageProcessorInVideoHeight;

    guint64 mCurPts;
    std::vector<TrackObjAttribute> mTrackObjVec; /* keep for tracking */
    std::vector<ObjectData> mObjectDataVec;  /* be used for output*/
    int mCurObjId = 0;		// Be used to assign new object id.

    // member
	std::vector<cv::Point2f> mPreFeaturePt;
	std::vector<cv::Point2f> mCurFeaturePt;

    // It is for Optical Flow algorithm
    GstBuffer *mPreFrame;

private:
    cv::UMat& get_umat(GstBuffer *buffer);
    cv::Mat get_mat(GstBuffer *buffer);
    std::vector<cv::Point2f> calc_feature_points(cv::UMat &gray);
    std::vector<cv::Point2f> find_points_in_ROI(cv::Rect roi);
    void calc_average_shift_moment(TrackObjAttribute& curObj, float& offx, float& offy);
    bool track_one_object(cv::Mat& curFrame, TrackObjAttribute& curObj, cv::Rect& outRoi);
    void figure_out_trajectory_points(ObjectData &objectVec, TrackObjAttribute& curObj);
    cv::Rect compare_detect_predict(std::vector<ObjectData>& objectVec, TrackObjAttribute& curObj, 
                                               cv::Rect predictRt, bool& bDetect);
    void get_roi_rect(cv::Rect& roiRect, cv::Rect curRect);
    void add_track_obj(CvdlAlgoData* &algoData, cv::Rect curRt, TrackObjAttribute& curObj, bool bDetect);
    void add_new_one_object(ObjectData &objectData, guint64 frameId);
    void add_new_objects(std::vector<ObjectData>& objectVec, guint64 frameId);
    bool is_at_buttom(TrackObjAttribute& curObj);
};


// Tracking object attribute.
class TrackObjAttribute
{
public:
	int objId;			// Object index
	uint64_t startFrameId;
	uint64_t curFrameId;
    float score;
    bool fliped; // whether it is fliped into next component.
    int detectedNum;//

	int notDetectNum;	/* The object is not detected in the continuous notDetectNum, stop tracking and report. */
	std::vector<cv::Rect> vecPos;	/* Every frame vehicle position based on tracking size */
	bool bBottom;	// Had arrived at image bottom; default = false;

	/**
	 * @brief Get current last vehicle for NVR show.
	 */
	cv::Rect getLastPos()
	{
		if (vecPos.size() > 0){
			return vecPos[vecPos.size() - 1];
		}
		return cv::Rect(0, 0, 0, 0);
	}

	/**
	 * @brief Calculate a mean shift position, if we track fail.
	 */
	void getLastShiftValue(float& shiftX, float& shiftY)
	{
		if (vecPos.size() >= 2) {
			int lastId = vecPos.size() - 2;
			shiftX = (vecPos[lastId + 1].x + vecPos[lastId + 1].width / 2)
					- (vecPos[lastId].x + vecPos[lastId].width / 2);
			shiftY = (vecPos[lastId + 1].y + vecPos[lastId + 1].height / 2)
					- (vecPos[lastId].y + vecPos[lastId].height / 2);
			shiftY = MAX(1, shiftY);
            SET_SATURATE(shiftX, -5, 5);
		}
		else{
			shiftX = 0;
			shiftY = 2;
		}
	}
};

#endif
