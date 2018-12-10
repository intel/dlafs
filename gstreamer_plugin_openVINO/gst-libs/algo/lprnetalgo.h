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

#ifndef __LPRNET_ALGO_H__
#define __LPRNET_ALGO_H__

#include "algobase.h"

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
                  if(!strncmp(lp.label.c_str(), item.label.c_str(), lp.label.size())) {
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

private:
    void ctc_ref_fp16(float* probabilities, float* output_sequences, float * output_prob,
                      int T_, int N_, int C_, int in_stride, bool original_layout);

    // The last algo should have an out queue
    //thread_queue<CvdlAlgoData> mOutQueue;
};

#endif
