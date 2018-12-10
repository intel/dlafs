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

#include <string>
#include "reidalgo.h"
#include "mathutils.h"
#include <ocl/oclmemory.h>
#include <ocl/crcmeta.h>
#include <ocl/metadata.h>
#include <pthread.h>
#include "algoregister.h"

using namespace HDDLStreamFilter;
using namespace std;

static void post_callback(CvdlAlgoData *algoData)
{
        // post process algoData
        ReidAlgo *reidAlgo = static_cast<ReidAlgo*> (algoData->algoBase);
        //int objNum = algoData->mObjectVec.size();

        float *descriptor = NULL;
        int matchedID = -1;
        PersonSet &personSet = reidAlgo->mPersonSet;

        bool bNeedFilterOut = true;
        bool bFilterOut = false;
        bool bAllPersonShown = true;

        reidAlgo->mPersonSetMutex.lock();
        // Search Person by descriptor.
        std::vector<ObjectData> &objectVec = algoData->mObjectVec;
        std::vector<ObjectData>::iterator it;
        for (it = objectVec.begin(); it != objectVec.end();) {
            ObjectData &objItem =(*it);
            descriptor = (float *)objItem.mAuxData;
            if(!descriptor) {
                g_print("Remove an object in reid!\n");
                delete (float *)(objItem.mAuxData);
                objItem.mAuxData = NULL;
                objItem.mAuxDataLen = 0;
                it = objectVec.erase(it);    //remove item.
                continue;
            }
            float prop = 0.0f;
            matchedID = personSet.findPersonByDescriptor(descriptor, &prop);

            if(matchedID == -1){
                // Add the newly appeared person into collection
                objItem.id = reidAlgo->mObjIndex++;
                personSet.addPerson(objItem.id, objItem.rectROI, descriptor);
                prop = 1.0f;
            }  else  {
                objItem.id = matchedID;
                personSet.updatePerson(objItem.id, objItem.rectROI, descriptor);
            }

            if(bNeedFilterOut) {
                if(matchedID==-1) {
                    bFilterOut  |= true;
                }else{
                    Person &person = personSet.getPerson(matchedID);
                    if(person.hitCount<10 || person.missCount>15)
                        bFilterOut  |= true;
                    if(person.rect.width < reidAlgo->mImageProcessorInVideoWidth/10
                        ||person.rect.height < reidAlgo->mImageProcessorInVideoHeight/4 )
                        bFilterOut  |= true;
                    if(prop<0.95)
                        bFilterOut  |= true;
                }
             }

            std::ostringstream stream_prob;
            stream_prob << "reid = "<< objItem.id;
            objItem.label = stream_prob.str();
            objItem.prob = prop;
            if(objItem.mAuxData) {
                delete (float *)(objItem.mAuxData);
                objItem.mAuxData = NULL;
                objItem.mAuxDataLen = 0;
            }
            ++it;
        }

        if(bNeedFilterOut) {
            if(bFilterOut) {
                objectVec.clear();
            } else {
                for(guint i=0;i<algoData->mObjectVec.size();i++) {
                    ObjectData &obj = algoData->mObjectVec[i];
                    bAllPersonShown &= personSet.getShowStatus(obj.id);
                    personSet.setShowStatus(obj.id);
                }
                if(bAllPersonShown)
                    objectVec.clear();
            }
        }
        personSet.update();
        reidAlgo->mPersonSetMutex.unlock();
}

ReidAlgo::ReidAlgo() : CvdlAlgoBase(post_callback, CVDL_TYPE_DL)
{
    mName = std::string(ALGO_REID_NAME);
    set_default_label_name();
}

ReidAlgo::~ReidAlgo()
{
    g_print("ReidAlgo: image process %d frames, image preprocess fps = %.2f, infer fps = %.2f\n",
        mFrameDoneNum, 1000000.0*mFrameDoneNum/mImageProcCost, 
        1000000.0*mFrameDoneNum/mInferCost);
}

void ReidAlgo::set_default_label_name()
{
     //set default label name
}

#if 0
void ReidAlgo::set_data_caps(GstCaps *incaps)
{
    std::string filenameXML;
    const gchar *env = g_getenv("HDDLS_CVDL_MODEL_PATH");
    if(env) {
        //($HDDLS_CVDL_MODEL_PATH)/<model_name>/<model_name>.xml
        filenameXML = std::string(env) + std::string("/") + mName + std::string("/") + mName + std::string(".xml");
    }else{
        filenameXML = std::string("HDDLS_CVDL_MODEL_PATH") + std::string("/") + mName
                                  + std::string("/") + mName + std::string(".xml");
        g_print("Error: cannot find %s model files: %s\n", mName.c_str(), filenameXML.c_str());
        exit(1);
    }
    algo_dl_init(filenameXML.c_str());
    init_dl_caps(incaps);
}
#endif

GstFlowReturn ReidAlgo::algo_dl_init(const char* modeFileName)
{
    GstFlowReturn ret = GST_FLOW_OK;

    // set in/out precision
    mIeLoader.set_precision(InferenceEngine::Precision::FP32, InferenceEngine::Precision::FP32);
    mIeLoader.set_mean_and_scale(0.0f,  1.0f);
    ret = init_ieloader(modeFileName, IE_MODEL_REID);

    return ret;
}

GstFlowReturn ReidAlgo::parse_inference_result(InferenceEngine::Blob::Ptr &resultBlobPtr,
                                                            int precision, CvdlAlgoData *outData, int objId)
{
    GST_LOG("ReidAlgo::parse_inference_result begin: outData = %p\n", outData);

    auto resultBlobFp32 = std::dynamic_pointer_cast<InferenceEngine::TBlob<float> >(resultBlobPtr);
    const size_t descriptorSize = 256;
    size_t resultSize = resultBlobPtr->size();

    if (descriptorSize != resultSize){
        g_print("descriptorSize = %ld, resultSize = %ld\n", descriptorSize , resultSize);
        return GST_FLOW_ERROR;
    }

    float *input = static_cast<float*>(resultBlobFp32->data());
    if (precision == sizeof(short)) {
        GST_ERROR("Don't support FP16!");
        return GST_FLOW_ERROR;
    }

    ObjectData object = outData->mObjectVecIn[objId];
    object.id = objId;

    object.mAuxDataLen = descriptorSize;
    float *descriptor  = new float[descriptorSize];
    object.mAuxData = (void *)descriptor;

    //parse inference result and put them into algoData
    #if 0
    for(size_t i = 0; i < descriptorSize; i ++){
        descriptor[i]=input[i];
    }
    #else
    memcpy(descriptor, input, descriptorSize*sizeof(float));
    #endif
    outData->mObjectVec.push_back(object);
    return GST_FLOW_OK;
}

/**************************************************************************
  *
  *    private method
  *
  **************************************************************************/
bool PersonSet::findPersonByID(int queryID)
{
    for(auto person : personVec){
        if(person.id == queryID){
            return true;
        }
    }
    return false;
}

int PersonSet::getPersonIndex(int id)
{
    int index = -1;
    for(guint i=0;i<personVec.size();i++){
        if(personVec[i].id == id){
            index = i;
        }
    }
    return index;
}

int PersonSet::findPersonByDescriptor(float *queryDescriptor, float *distance)
{
    int mostSimilarID = -1;
    float maxSimilarity = -1;
    MathUtils mathUtils;

    for(auto person : personVec){
        int id = person.id;
        float similarity = mathUtils.cosDistance(queryDescriptor, person.descriptor, 255);
        if(similarity > maxSimilarity){
            maxSimilarity = similarity;
            mostSimilarID = id;
        }
    }

    *distance = maxSimilarity;
    if(maxSimilarity > 0.4f){
        return mostSimilarID;
    }else{
        return -1;
    }
}

void PersonSet::addPerson(int id, cv::Rect rect, float * descriptor)
{
    Person person;
    person.id = id;
    person.rect = rect;
    person.hitCount = 1;
    person.hit = true;
    person.missCount=0;
    person.successionMissCount = 0;

    for(int i = 0; i < 256; i ++){
        person.descriptor[i] = descriptor[i];
    }
    personVec.push_back(person);
}

Person& PersonSet::getPerson(int id)
{
    static Person empty_person = Person(); 
    int index = getPersonIndex(id);
    if(index==-1)
        return empty_person;
    return  personVec[index];
}

void PersonSet::setShowStatus(int id)
{
    int index = getPersonIndex(id);
    if(index==-1)
        return ;
    personVec[index].bShow = true;;
    return;
}

bool PersonSet::getShowStatus(int id)
{
    int index = getPersonIndex(id);
    if(index==-1)
        return false ;

    return personVec[index].bShow;
}

void PersonSet::updatePerson(int id, cv::Rect rect, float * descriptor)
{
    int index = getPersonIndex(id);
    if(index==-1)
        return;

    Person &person = personVec[index];
    person.rect.x = (person.rect.x *3 + rect.x)/4;
    person.rect.y = (person.rect.y *3 + rect.y)/4;
    person.rect.width = (person.rect.width *3 + rect.width)/4;
    person.rect.height = (person.rect.height *3 + rect.height)/4;
    person.hitCount++;
    person.successionMissCount = 0;
    person.hit = true;

    for(int i = 0; i < 256; i ++){
        person.descriptor[i] = (person.descriptor[i]*3 + descriptor[i])/4;
    }
}

void PersonSet::update()
{
    std::vector<Person>::iterator it;
    for (it = personVec.begin(); it != personVec.end();) {
        if((*it).hit==false) {
            (*it).missCount++;
            (*it).successionMissCount++;
        }
        (*it).hit=false;
        if((*it).missCount>300)
             it = personVec.erase(it);    //remove item.
         else
             ++it;
    }
}
