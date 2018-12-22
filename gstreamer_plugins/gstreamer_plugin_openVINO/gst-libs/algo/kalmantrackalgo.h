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

#ifndef __MINI_TRACK_ALGO_H__
#define __MINI_TRACK_ALGO_H__

#include <vector>
#include "algobase.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include <gst/gstpad.h>
#include <opencv2/opencv.hpp>
#include "ieloader.h"

#include "kalman.h"

#define SET_SATURATE(X, L, H) \
        if(X<L) X=L; \
        else if(X>H) X=H;

// Tracking object attribute.
class TrackLpObjAttribute
{
public:
    int objId;      // Object index
    uint64_t startFrameId;
    uint64_t curFrameId;
    float score;
    bool fliped; // whether it is fliped into next component.
    int detectedNum;//
    cv::Rect rect; // vehicle rect of this frame
    gboolean bHit;// if be hit in new frame

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
};

class KalmanTrackAlgo : public CvdlAlgoBase 
{
public:
    KalmanTrackAlgo();
    virtual ~KalmanTrackAlgo();
    virtual void set_data_caps(GstCaps *incaps);
    void verify_detection_result(std::vector<ObjectData> &objectVec);
    void try_add_new_one_object(ObjectData &objectData, guint64 frameId);
    void verify_tracked_object();
    bool is_at_buttom(TrackLpObjAttribute& curObj);
    void remove_invalid_object(std::vector<ObjectData> &objectVec);
    void update_track_object(std::vector<ObjectData> &objectVec);
    //void push_track_object(CvdlAlgoData* &algoData);

    guint64 mCurPts;
    std::vector<TrackLpObjAttribute> mTrackObjVec; /* keep for tracking */
    std::vector<ObjectData> mObjectDataVec;  /* be used for output*/
    int mCurObjId = 0;  // Be used to assign new object id.

    std::vector<ObjectData> mLastObjectTrackRes; // last fame track result

    KalmanTracker *kalmanTracker;
    std::string mSvmModelStr;

private:
    //It's not expected that class instances are copied, the operator= should be declared as private.
    //In this case, if an attempt to copy is made, the compiler produces an error.
    KalmanTrackAlgo& operator=(const KalmanTrackAlgo& src){return *this;}
};


#endif
