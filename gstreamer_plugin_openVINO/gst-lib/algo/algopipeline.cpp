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

// wrapper C++ algo pipeline, so that it can be called by C
//
// This file will be compiled by C++ compiler
//

#include "detectionalgo.h"
#include "trackalgo.h"
#include "classificationalgo.h"
#include "algopipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

static AlgoPipelineConfig algoTopologyDefault[] = {
    {0, ALGO_DETECTION,      -1,  1, {1}},
    {1, ALGO_TRACKING,        0,  1, {2}},
    {2, ALGO_CLASSIFICATION,  1,  1, {-1}},
};

static CvdlAlgoBase* algo_create(int type)
{
    CvdlAlgoBase* algo;
    switch(type) {
        case ALGO_DETECTION:
            algo = new DetectionAlgo;
            break;
        case ALGO_TRACKING:
            algo = new TrackAlgo;
            break;
        case ALGO_CLASSIFICATION:
            algo = new ClassificationAlgo;
            break;
        default:
            algo = NULL;
            break;
   };

   return algo;
}

//TODO: need support one algo link to multiple algos,  specified the algo's src pad 
static void algo_item_link(AlgoItem* from, AlgoItem* to)
{
    CvdlAlgoBase *algoFrom, *algoTo;
    if(!from || !to || !from->algo || !to->algo) {
        GST_ERROR("algo link failed!\n");
        return;
    }
    algoFrom = static_cast<CvdlAlgoBase *>(from->algo);
    algoTo   = static_cast<CvdlAlgoBase *>(to->algo);
    algoFrom->algo_connect(algoTo);
}

AlgoPipelineHandle algo_pipeline_create(AlgoPipelineConfig* config, int num)
{
    AlgoPipelineHandle handle = 0;
    AlgoPipeline *pipeline = (AlgoPipeline *)g_new0(AlgoPipeline,1);
    AlgoItem *item = NULL;
    int preId, nextId = 0;

    pipeline->algo_chain = (AlgoItem *)g_new0(AlgoItem, num);
    pipeline->algo_num   = num;
    pipeline->first = NULL;

    int i, j;
    // set id/type/algo/nextItem/preItem for every Item
    for(i=0; i< num; i++) {
        item = pipeline->algo_chain + i;
        item->id = config[i].curId;
        item->type = config[i].curType;
        item->algo = static_cast<void *>(algo_create(config[i].curType));
        preId = config[i].preId;
        if(preId>=0){
            item->preItem = pipeline->algo_chain + preId;
        }else {
            item->preItem = NULL;
        }
        for(j=0;j<config[i].nextNum;j++) {
            nextId = config[i].nextId[j];
            if(nextId>=0){
                item->nextItem[j] = pipeline->algo_chain + nextId;
            }else {
                item->nextItem[j] = NULL;
            }
        }
    }

    // link algo and get first and last algo
    for(i=0; i< num; i++) {
        item = pipeline->algo_chain + i;
        if(item->preItem){
            algo_item_link(item->preItem, item);
        }else {
            // here preId = -1, means it is the first algo item
            // Note: we should only have one first algo
            pipeline->first = item->algo;
        }
        for(j=0;j<config[i].nextNum;j++) {
            nextId = config[i].nextId[j];
            if(nextId>=0){
                // next algo has not been created
                //algo_item_link(item, item->nextItem[j]);
            }else {
                nextId = (-1 * nextId) - 1;
                pipeline->last[nextId] = item->algo;
            }
        }
    }

    handle = (AlgoPipelineHandle)pipeline;
    return handle;
}

AlgoPipelineHandle algo_pipeline_create_default()
{
    return algo_pipeline_create(algoTopologyDefault, 
        sizeof(algoTopologyDefault)/sizeof(AlgoPipelineConfig));
}
void algo_pipeline_destroy(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    int i;
    CvdlAlgoBase* algo;
    if(pipeline==NULL) {
        GST_ERROR("%s - algo pipeline handle is NULL!\n", __func__);
        return;
    }
    for(i=0; i< pipeline->algo_num; i++){
        algo = static_cast<CvdlAlgoBase *>(pipeline->algo_chain[i].algo);
        delete algo;
    }
    g_free(pipeline->algo_chain);
    g_free(pipeline);
}
void algo_pipeline_set_caps(AlgoPipelineHandle handle, int algo_id, GstCaps* caps)
{
     AlgoPipeline *pipeline = (AlgoPipeline *) handle;
     CvdlAlgoBase* algo = NULL;
     if(pipeline==NULL) {
         GST_ERROR("%s - algo pipeline handle is NULL!\n", __func__);
         return;
     }

     if(algo_id>=pipeline->algo_num) {
        GST_ERROR("algo_id is invalid: %d", algo_id);
        return;
     }

    algo = static_cast<CvdlAlgoBase *>(pipeline->algo_chain[algo_id].algo);
    algo->set_data_caps(caps);
}

void algo_pipeline_set_caps_all(AlgoPipelineHandle handle, GstCaps* caps)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;
    int i;
    if(pipeline==NULL) {
        GST_ERROR("%s - algo pipeline handle is NULL!\n", __func__);
        return;
    }
    for(i=0; i< pipeline->algo_num; i++){
        algo = static_cast<CvdlAlgoBase *>(pipeline->algo_chain[i].algo);
        algo->set_data_caps(caps);
    } 
}


void algo_pipeline_start(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;

    int i;
    if(pipeline==NULL) {
        GST_ERROR("%s - algo pipeline handle is NULL!\n", __func__);
        return;
    }
    for(i=0; i< pipeline->algo_num; i++){
        algo = static_cast<CvdlAlgoBase *>(pipeline->algo_chain[i].algo);
        algo->start_algo_thread();
    }
}
void algo_pipeline_stop(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;
    int i;

    if(pipeline==NULL) {
        GST_ERROR("%s - algo pipeline handle is NULL!\n", __func__);
        return;
    }

    for(i=0; i< pipeline->algo_num; i++){
        algo = static_cast<CvdlAlgoBase *>(pipeline->algo_chain[i].algo);
        algo->stop_algo_thread();
    }
}
void algo_pipeline_put_buffer(AlgoPipelineHandle handle, GstBuffer *buf)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;

    if(pipeline==NULL) {
        GST_ERROR("algo pipeline handle is NULL!\n");
        return;
    }
    algo = static_cast<CvdlAlgoBase *>(pipeline->first);
    if(!algo){
        GST_ERROR("failed to put_buffer: algo is NULL");
        return;
    }
    //g_print("%s() - GstBuffer = %p\n",__func__,  buf);
    algo->queue_buffer(buf);
}


int algo_pipeline_get_input_queue_size(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;

    if(pipeline==NULL) {
        GST_ERROR("algo pipeline handle is NULL!\n");
        return 0;
    }
    algo = static_cast<CvdlAlgoBase *>(pipeline->first);
    if(!algo){
        GST_ERROR("failed to put_buffer: algo is NULL");
        return 0;
    }

    return algo->get_in_queue_size();
}


int algo_pipeline_get_all_queue_size(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;
    int size = 0;

    if(pipeline==NULL) {
        GST_ERROR("algo pipeline handle is NULL!\n");
        return 0;
    }
    algo = static_cast<CvdlAlgoBase *>(pipeline->first);
    while(algo) {
        size += algo->get_in_queue_size() + algo->get_out_queue_size();
        size += algo->mInferCnt;
        algo=algo->mNext;
    }

    return size;
}


void algo_pipeline_get_buffer(AlgoPipelineHandle handle, GstBuffer **buf)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;

    //TODO: need support multiple output buffer
    if(pipeline) {
        algo = static_cast<CvdlAlgoBase *>(pipeline->last[0]);
        if(algo)
            *buf = algo->dequeue_buffer();
        else
            *buf = NULL;
    }
}

void algo_pipeline_flush_buffer(AlgoPipelineHandle handle)
{
    AlgoPipeline *pipeline = (AlgoPipeline *) handle;
    CvdlAlgoBase* algo = NULL;

    //TODO: need support multiple output buffer
    if(pipeline) {
        algo = static_cast<CvdlAlgoBase *>(pipeline->last[0]);
        g_print("%s() - put EOS buffer!\n",__func__);
        // send a empty buffer to make this get_buffer_task exit 
        algo->queue_out_buffer(NULL);
    }
}
#ifdef __cplusplus
};
#endif

