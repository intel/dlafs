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

/*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

#ifndef __GENERIC_ALGO_H__
#define __GENERIC_ALGO_H__

#include "algobase.h"
#include "ieloader.h"
#include "imageproc.h"
#include <gst/gstbuffer.h>
#include "mathutils.h"
#include "exinferdata.h"

class GenericAlgo : public CvdlAlgoBase 
{
public:
    GenericAlgo(const char *name);
    virtual ~GenericAlgo();
    //virtual void set_data_caps(GstCaps *incaps);
    virtual GstFlowReturn algo_dl_init(const char* modeFileName);
    virtual GstFlowReturn parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                 int precision, CvdlAlgoData *outData, int objId);
    void set_default_label_name();
    void set_label_names(const char** label_names){mLabelNames = label_names;}

    void exobjdata_2_objdata(ExObjectData &exObjData,  ObjectData &objData);
    void objdata_2_exobjdata(ObjectData &objData,  ExObjectData &exObjData);
    InferenceEngine::Precision getIEPrecision(ExDataType type) ;

    //std::string mName;
    void *mHandler;
    pfInferenceResultParseFunc pfParser;
    pfPostProcessInferenceDataFunc pfPostProcess;
    pfGetDataTypeFunc pfGetType;
    pfGetMeanScaleFunc pfGetMS;
    pfGetNetworkConfigFunc pfGetNetworkConfig;

    bool mLoaded; // If dynamic library can be loaded
    std::string mLibName;

    ExDataType inType;
    ExDataType outType;

    guint64 mCurPts;

    const char** mLabelNames;
};
#endif

