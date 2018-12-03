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

#include "algoregister.h"
#include <string.h>
#include <assert.h>
#include <glib.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static AlgoList g_algo_list = {.algo_num=0, .generic_algo_num=0,};

void register_add_algo(int id, const char* name)
{
    int i=0;

    if(g_algo_list.algo_num>=ALGO_LIST_ITEM_MAX_NUM) {
           g_print("Error: algo register is full!\n");
           return;
    }

    for(i=0;i<g_algo_list.algo_num; i++) {
        if((g_algo_list.algo_item[i].id==id) ||
             ( !strncmp(g_algo_list.algo_item[i].name, name,strlen(g_algo_list.algo_item[i].name) ) &&
                !strncmp(g_algo_list.algo_item[i].name, name,strlen(name))) ){
              g_print("Algo(id=%d, name=%s) has existed!\n",id, name);
            return ;
        }
    }
     g_algo_list.algo_item[g_algo_list.algo_num].id = id;
     memcpy(g_algo_list.algo_item[g_algo_list.algo_num].name, name, strlen(name));
     g_algo_list.algo_num++;
}

//TODO: make id succession if failed to add algo
int register_get_free_algo_id()
{
        int id =  g_algo_list.generic_algo_num + ALGO_MAX_DEFAULT_NUM;
        g_algo_list.generic_algo_num++;
        return id;
}

 char *register_get_algo_name(int id)
{
    int i=0;
    for(i=0;i<g_algo_list.algo_num;i++) {
        if(g_algo_list.algo_item[i].id==id)
            return g_algo_list.algo_item[i].name;
    }
    g_print("Failed to get algo name, id = %d\n",id);
    return NULL;
}

int register_get_algo_id(const char *name)
{
    int i=0;
    for(i=0;i<g_algo_list.algo_num;i++) {
        if(strlen(name) != strlen(g_algo_list.algo_item[i].name))
            continue;
        if(!strncmp(name, g_algo_list.algo_item[i].name, strlen(name))) {
            return g_algo_list.algo_item[i].id;
        }
    }
    g_print("Failed to get algo id, algo name = %s\n",name);
    return -1;
}

 static void register_init_default_algo()
{
     register_add_algo(ALGO_DETECTION,  ALGO_DETECTION_NAME);
     register_add_algo(ALGO_TRACKING,  ALGO_TRACKING_NAME);
     register_add_algo(ALGO_CLASSIFICATION,  ALGO_CLASSIFICATION_NAME);
     register_add_algo(ALGO_SSD,  ALGO_SSD_NAME);
     register_add_algo(ALGO_TRACK_LP,  ALGO_TRACK_LP_NAME);
     register_add_algo(ALGO_REGCONIZE_LP,  ALGO_RECOGNIZE_LP_NAME);
     register_add_algo(ALGO_YOLO_TINY_V2,  ALGO_YOLO_TINY_V2_NAME);
     register_add_algo(ALGO_REID,  ALGO_REID_NAME);
     register_add_algo(ALGO_SINK,  ALGO_SINK_NAME);
}

void register_write()
{
    FILE *pf = fopen(ALGO_REGISTER_FILE_NAME,"wb");
    if(pf) {
        fwrite(&g_algo_list, sizeof(AlgoList), 1, pf);
        fclose(pf);
    }
}

int  register_read()
{
    FILE *pf = fopen(ALGO_REGISTER_FILE_NAME,"rb");
    if(pf) {
        fread(&g_algo_list, sizeof(AlgoList), 1, pf);
        fclose(pf);
        return 1;
    }
    return 0;
}

void register_init()
{
    static int flag = 0;
    if(flag) return;
    flag = 1;

    if(!register_read()) {
        register_init_default_algo();
    }
}

void register_dump()
{
    int i=0;
    g_print("algo list:\n");
    for(i=0;i<g_algo_list.algo_num;i++) {
        g_print("\tid = %d, name = %s\n", g_algo_list.algo_item[i].id,  g_algo_list.algo_item[i].name);
    }
}
void register_reset()
{
    memset(&g_algo_list, 0x0, sizeof(AlgoList));
}

#ifdef __cplusplus
};
#endif
