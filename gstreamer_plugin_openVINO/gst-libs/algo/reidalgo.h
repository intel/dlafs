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
#ifndef __REID_ALGO_H__
#define __REID_ALGO_H__

#include "algobase.h"
#include "ieloader.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include "mathutils.h"

class Person{
public:
    Person(): id(-1), hitCount(-1), missCount(-1),
        successionMissCount(0), hit(false), bShow(false) {}
    int id;
    cv::Rect rect;
    float descriptor[256];
    gint hitCount;
    gint missCount;
    gint successionMissCount;
    gboolean hit;
    gboolean bShow; // if has been shown
};

class PersonSet
{
public:
    PersonSet() {}
    ~PersonSet()
   {
        personVec.clear();
   }
    guint getPersonNum() { return personVec.size(); }
    bool findPersonByID(int queryID);
    int findPersonByDescriptor(float * queryDescriptor , float *distance);
    void addPerson(int id, cv::Rect rect, float * descriptor);
    int getPersonIndex(int id);
    void updatePerson(int id, cv::Rect rect, float * descriptor);
    Person& getPerson(int id);
    void setShowStatus(int id);
    bool getShowStatus(int id);
    void update();
    std::vector<Person> personVec;
};

class ReidAlgo : public CvdlAlgoBase 
{
public:
    ReidAlgo();
    virtual ~ReidAlgo();
    //virtual void set_data_caps(GstCaps *incaps);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId);
    void set_default_label_name();
    void set_label_names(const char** label_names){mLabelNames = label_names;}

    PersonSet mPersonSet;
    std::mutex mPersonSetMutex;
    const char **mLabelNames;
};
#endif

