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

