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
#ifndef __CVDL_ALGO_REGISTER_H__
#define __CVDL_ALGO_REGISTER_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ALGO_NONE      = -1,
    ALGO_DETECTION                = 0,
    ALGO_TRACKING                  = 1,
    ALGO_CLASSIFICATION     = 2,
    ALGO_SSD                                = 3,
    ALGO_TRACK_LP                  =4,
    ALGO_REGCONIZE_LP         = 5,
    ALGO_YOLO_TINY_V2         = 6,
    ALGO_REID                               = 7,
    ALGO_SINK,   /*last algo in algopipe*/
    ALGO_MAX_DEFAULT_NUM,
};

#define ALGO_DETECTION_NAME "detection"
#define ALGO_TRACKING_NAME "track"
#define ALGO_CLASSIFICATION_NAME "classification"
#define ALGO_SSD_NAME "ssd"
#define ALGO_TRACK_LP_NAME "tracklp"
#define ALGO_RECOGNIZE_LP_NAME "lprecognize"
#define ALGO_YOLO_TINY_V2_NAME "yolotinyv2"
#define ALGO_REID_NAME "reid"
#define ALGO_GENERIC_DL_NAME "generic"
#define ALGO_SINK_NAME "sink"

#define ALGO_REGISTER_FILE_NAME  ".algocache.register"

struct _AlgoListItem {
    int id;
    char name[32];
};
typedef struct _AlgoListItem AlgoListItem;
#define ALGO_LIST_ITEM_MAX_NUM 32

typedef struct _AlgoList{
    int algo_num;
    int generic_algo_num; // number of generic algo
    AlgoListItem algo_item[ALGO_LIST_ITEM_MAX_NUM];
}AlgoList;

void register_add_algo(int id, const char* name);
char *register_get_algo_name(int id);
int register_get_algo_id(const char *name);
int register_get_free_algo_id();
void register_init();
int register_read();
void register_write();
void register_reset();
void register_dump();

#ifdef __cplusplus
};
#endif

#endif
