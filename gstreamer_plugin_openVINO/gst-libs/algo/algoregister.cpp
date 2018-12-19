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
#include <assert.h>
#include <glib.h>
#include <gst/gst.h>

using namespace std;
#include <string>
#include<iostream>
#include<cstdio>
#include <fstream>

//#ifdef __cplusplus
//extern "C" {
//#endif

void AlgoRegister::add_algo(int id, const char* name)
{
    // check if this name/id has already exist.
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        if(!item.name_str.compare(name) || item.id==id) {
            g_print("Algo(id=%d, name=%s) has existed!\n",id, name);
            return;
        }
    }

    AlgoListItem new_item;
    new_item.id = id;
    new_item.name_str = std::string(name);
    algo_vec.push_back(new_item);
    return;
}

int AlgoRegister::get_free_id()
 {
    return ALGO_MAX_DEFAULT_NUM + generic_algo_num++;
 }

const char *AlgoRegister::get_algo_name(int id)
{
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        if(id == item.id) {
            return item.name_str.c_str();
        }
    }

    g_print("Failed to get algo name, id = %d\n",id);
    return NULL;
}

int AlgoRegister::get_algo_id(const char *name)
{
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        if(!item.name_str.compare(name)) {
            return item.id;
        }
    }
    g_print("Failed to get algo id, algo name = %s\n",name);
    return -1;
}

 void AlgoRegister::init_default_algo()
{
     add_algo(ALGO_YOLOV1_TINY,  ALGO_YOLOV1_TINY_NAME);
     add_algo(ALGO_OF_TRACK,  ALGO_OF_TRACK_NAME);
     add_algo(ALGO_GOOGLENETV2,  ALGO_GOOGLENETV2_NAME);
     add_algo(ALGO_MOBILENET_SSD,  ALGO_MOBILENET_SSD_NAME);
     add_algo(ALGO_TRACK_LP,  ALGO_TRACK_LP_NAME);
     add_algo(ALGO_LPRNET,  ALGO_LPRNET_NAME);
     add_algo(ALGO_YOLOV2_TINY,  ALGO_YOLOV2_TINY_NAME);
     add_algo(ALGO_REID,  ALGO_REID_NAME);
     add_algo(ALGO_SINK,  ALGO_SINK_NAME);
}

void AlgoRegister::register_write()
{
    std::ofstream ofs;
     ofs.exceptions ( std::ifstream::failbit |std::ifstream::badbit );
     try {
        ofs.open (ALGO_REGISTER_FILE_NAME, std::ofstream::out);
        for(unsigned int i=0; i<algo_vec.size();i++) {
            AlgoListItem &item = algo_vec[i];
            ofs << item.id <<std::endl;
            ofs << item.name_str.c_str() << std::endl;
        }
        ofs.close();
     }
     catch (std::ifstream::failure e) {
       std::cerr << "Exception opening/writing/closing file\n";
     }
}

int  AlgoRegister::register_read()
{
    std::ifstream ifs (ALGO_REGISTER_FILE_NAME);
    algo_vec.clear();

    // File didn't exist, return 0.
    if(!ifs.good())
        return 0;

    try {
       //ifs.open (ALGO_REGISTER_FILE_NAME, std::ifstream::in);
       ifs.exceptions ( std::ifstream::failbit |std::ifstream::badbit  | std::ifstream::eofbit);
      while (!ifs.eof()) {
           AlgoListItem item;
           ifs >>  item.id ;
           ifs >> item.name_str;
           algo_vec.push_back(item);
       }
       ifs.close();
    }
    catch (std::ifstream::failure e) {
      //std::cerr << "Exception opening/reading/closing file!\n" ;
       ifs.close();
    }

    if(algo_vec.size()>0)
        return 1;
    else
        return 0;
}

void AlgoRegister::register_init()
{
    if(inited) return;
    inited = true;

    if(!register_read()) {
        init_default_algo();
        register_write();
    }

    //update generic_algo_num
    generic_algo_num = 0;
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        if(item.id>=ALGO_MAX_DEFAULT_NUM)
            generic_algo_num++;
     }
}

void AlgoRegister::register_dump()
{
    g_print("algo list:\n");
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        g_print("\tid = %d, name = %s\n", item.id,  item.name_str.c_str());
    }
    g_print("\n");
}
void AlgoRegister::register_reset()
{
    algo_vec.clear();
    generic_algo_num = 0;
    inited = false;
    if(remove(ALGO_REGISTER_FILE_NAME))
        g_print("Success to remove %s\n", ALGO_REGISTER_FILE_NAME);
    else
        g_print("Failed to remove %s\n", ALGO_REGISTER_FILE_NAME);
}

//#ifdef __cplusplus
//};
//#endif
