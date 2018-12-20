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

 #include "kalman.h"

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
//  KalmanBoxTracker && KalmanTracker
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

void KalmanTracker::getPredicts(BBoxArray & predicts)
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

void KalmanTracker::updateMatchingSets(const std::map<long, long> & predToDetMatching)
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

void KalmanTracker::updateMatchedTrackers(const std::map<long, long> & predictToDetMatching,
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


void KalmanTracker::updateUnmatchedTrackers()
{
    for(long trackIdx = 0; trackIdx < (long)(this->KFBoxTrackers.size()); trackIdx ++) {
        if (matchedPredicts.find(trackIdx) == matchedPredicts.end()) {
            KalmanBoxTracker & boxTracker = this->KFBoxTrackers[trackIdx];
            boxTracker.updateWithPredict();
        }
    }
}

void KalmanTracker::addNewTrackers(const BBoxArray & dets)
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


void KalmanTracker::removeInvalidTrackers(int imageWidth, int imageHeight)
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

void KalmanTracker::getCurrentState(BBoxArrayWithId & results)
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

BBoxArrayWithId KalmanTracker::update(const BBoxArray & dets, int width, int height)
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


void KalmanTracker::recoverID(int currentID, int targetID)
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

bool KalmanTracker::findObjectByID(int queryID)
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

