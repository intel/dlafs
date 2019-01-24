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

#ifndef __KALMAN_ALGO_H__
#define __KALMAN_ALGO_H__

#include <opencv2/opencv.hpp>
#include "eigen3/Eigen/Dense"
#include "dlib/optimization.h"

class KFTrack {
public:
  KFTrack(
      const Eigen::MatrixXd& A,
      const Eigen::MatrixXd& C,
      const Eigen::MatrixXd& Q,
      const Eigen::MatrixXd& R,
      const Eigen::MatrixXd& P,
      const Eigen::VectorXd& initState
  );

  KFTrack() {}

  Eigen::VectorXd predict();
  void updateWithPredicted();
  void update(const Eigen::VectorXd& observed);
  Eigen::VectorXd state() { return xHat; }

private:
  //System
  Eigen::MatrixXd A, C, Q, R, P, K;

  //States
  Eigen::VectorXd xHat, xHatNew;

  //Identity matrix
  Eigen::MatrixXd I;
};

class KalmanBoxTracker
{
public:
    KalmanBoxTracker(int id, std::vector<double> bbox);
    void update(std::vector<double> bbox);
    std::vector<double> predict();
    void updateWithPredict();
    
    std::vector<double> getState();
    int getTimeSinceUpdate() { return timeSinceUpdate; }
    int getId() {return id; }
    void changeID(int id) {this->id = id; }

private:
    KFTrack kf;
    int dimX;
    int dimZ;
    int timeSinceUpdate;
    int id;
    std::vector<std::vector<double> > history;
    int hits;
    int hitStreak;
};

typedef std::vector<double> BBox; // (x1, x2, y1, y2)
typedef std::vector<int> BBoxWithId; // (x1, x2, y1, y2, id)
typedef std::vector<BBox> BBoxArray;
typedef std::vector<BBoxWithId> BBoxArrayWithId;

BBox convertOcvBBoxToBBox(const cv::Rect2d & rect);
cv::Rect2d convertBBoxToOcvBBox(const BBox & bbox);

class KalmanTracker
{
public:
    KalmanTracker() : maxAge(12), count(0) {}
    KalmanTracker(int maxAge) : maxAge(maxAge), count(0) {}

    /**
     * @brief update
     * @param dets: current detection bboxes, (x1, y1, x2, y2) ...
     * @param frame: current frame
     * @return a vector of bboxes with object id, (x1, y1, x2, y2, id) ...
     */
    BBoxArrayWithId update(const BBoxArray & dets, int width, int height);
    void recoverID(int currentID, int targetID);
    bool findObjectByID(int queryID);
    void getCurrentState(BBoxArrayWithId &results);

    const std::map<long, long> & getBoxToObjectMapping()  const { return detToObject; }

private:
    int maxAge;
    int count;

    std::vector<KalmanBoxTracker> KFBoxTrackers;

    std::set<long> matchedPredicts;
    std::set<long> matchedDets;

    std::map<long, long> detToObject; // the current mapping from det box id to tracked object id.

    void getPredicts(BBoxArray & predicts);

    void updateMatchingSets(const std::map<long, long> & predToDetMatching);
    void updateMatchedTrackers(const std::map<long, long> & predToDetMatching,
                               const BBoxArray & predicts, const BBoxArray & dets);

    void updateUnmatchedTrackers();
    void addNewTrackers(const BBoxArray & dets);
    void removeInvalidTrackers(int imageWidth, int imageHeight);

};

#endif
