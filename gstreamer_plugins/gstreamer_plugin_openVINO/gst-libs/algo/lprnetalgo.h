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

#ifndef __LPRNET_ALGO_H__
#define __LPRNET_ALGO_H__

#include "algobase.h"

#define LPR_ROWS 71
#define LPR_COLS 88

class LicencePlateData
{
public:
    LicencePlateData() :prob(0.0),rect(cv::Rect(0,0,0,0)),life(0){};
   LicencePlateData(std::string l, float p, cv::Rect r ) :label(l),prob(p),rect(r){};
    std::string label;
    float prob;
    cv::Rect rect;
    int life;
};

class LicencePlatePool
{
    public:
        LicencePlatePool(){}
        ~LicencePlatePool()
       {
            mLPVec.clear();
        }

        // return true - new LP
        // return false - invalide LP or dup LP
        bool check(LicencePlateData &lp)
        {
              if(lp.prob<0.5)
                 return FALSE;

              bool hit = FALSE;
              int num = mLPVec.size();
              for(int i=0; i<num; i++) {
                  LicencePlateData &item =mLPVec[i];
                  if(!lp.label.compare(item.label)) {
                        //hit it
                        item.life=0;
                        hit = TRUE;
                        break;
                  }
              }

              if(hit) return FALSE;

              mLPVec.push_back(lp);
              return TRUE;
        }

        void update()
        {
            std::vector<LicencePlateData>::iterator it;
            for (it = mLPVec.begin(); it != mLPVec.end();) {
                (*it).life++;
                if((*it).life  >= 80) {
                    // remove  it
                    it = mLPVec.erase(it);
                 } else {
                    it++;
                 }
            }
        }
    private:
        std::vector<LicencePlateData> mLPVec;
};

class LPRNetAlgo : public CvdlAlgoBase 
{
public:
    LPRNetAlgo();
    virtual ~LPRNetAlgo();

    //virtual void set_data_caps(GstCaps *incaps);
    //virtual GstBuffer* dequeue_buffer();
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                      int precision, CvdlAlgoData *outData, int objId);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);

    guint64 mCurPts;
    LicencePlatePool lpPool;
    float mSecData[LPR_COLS];
};

#endif
