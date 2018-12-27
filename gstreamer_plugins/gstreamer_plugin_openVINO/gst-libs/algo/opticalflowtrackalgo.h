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

#ifndef __OPTICAL_FLOW_TRACK_ALGO_H__
#define __OPTICAL_FLOW_TRACK_ALGO_H__

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

class OpticalflowTrackAlgo : public CvdlAlgoBase 
{
public:
    OpticalflowTrackAlgo();
    virtual ~OpticalflowTrackAlgo();
    virtual void set_data_caps(GstCaps *incaps);

    void verify_detection_result(std::vector<ObjectData> &objectVec);
    void track_objects(CvdlAlgoData* &algoData);
    void track_objects_fast(CvdlAlgoData* &algoData);
    //void push_track_object(CvdlAlgoData* &algoData);
    void update_track_object(std::vector<ObjectData> &objectVec);

    guint64 mCurPts;
    std::vector<TrackObjAttribute> mTrackObjVec; /* keep for tracking */
    std::vector<ObjectData> mObjectDataVec;  /* be used for output*/
    int mCurObjId = 0; // Be used to assign new object id.

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
    int objId;      // Object index
    uint64_t startFrameId;
    uint64_t curFrameId;
    float score;
    bool fliped; // whether it is fliped into next component.
    int detectedNum;//

    int notDetectNum;   /* The object is not detected in the continuous notDetectNum, stop tracking and report. */
    std::vector<cv::Rect> vecPos;/* Every frame vehicle position based on tracking size */
    bool bBottom;// Had arrived at image bottom; default = false;

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
        } else {
            shiftX = 0;
            shiftY = 2;
        }
    }
};

#endif
