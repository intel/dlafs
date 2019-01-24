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

#include "algoregister.h"
#include <assert.h>
#include <glib.h>
#include <gst/gst.h>

using namespace std;
#include <string>
#include<iostream>
#include<cstdio>
#include <fstream>
#include <vector>

class AlgoListItem {
public:
    int id;
    std::string name_str;
};

class AlgoRegister{
public:
    AlgoRegister():  generic_algo_num(0), algo_id(0), inited(false)
   {
   };
    ~AlgoRegister(){};
    void add_algo(int id, const char* name);
    const char *get_algo_name(int id);
    int get_algo_id(const char *name);
    int get_free_id();
    void register_init();
    int register_read();
    void register_write();
    void register_reset();
    void register_dump();

    int get_algo_num()
    {
        return algo_vec.size();
     }
    int get_generic_algo_num()
    {
        return generic_algo_num;
    }
private:
    void init_default_algo();
    std::vector<AlgoListItem>  algo_vec;
    int generic_algo_num; // number of generic algo
    int algo_id;
    bool inited;
};

void AlgoRegister::add_algo(int id, const char* name)
{
    // check if this name/id has already exist.
    for(unsigned int i=0; i<algo_vec.size();i++) {
        AlgoListItem &item = algo_vec[i];
        if(!item.name_str.compare(name) || item.id==id) {
            GST_ERROR("Algo(id=%d, name=%s) has existed!\n",id, name);
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

    GST_ERROR("Failed to get algo name, id = %d\n",id);
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
    GST_ERROR("Failed to get algo id, algo name = %s\n",name);
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
        GST_ERROR("Exception opening/writing/closing file\n");
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
        GST_INFO("Success to remove %s\n", ALGO_REGISTER_FILE_NAME);
    else
        GST_INFO("Failed to remove %s\n", ALGO_REGISTER_FILE_NAME);
}

static AlgoRegister g_algoRegister;

#ifdef __cplusplus
extern "C" {
#endif

void register_add_algo(int id, const char* name)
{
    g_algoRegister.add_algo(id,name);
}
const char *register_get_algo_name(int id)
{
    return g_algoRegister.get_algo_name(id);
}
 int register_get_algo_id(const char *name)
{
    return g_algoRegister.get_algo_id(name);
}
 int register_get_free_algo_id()
{
    return g_algoRegister.get_free_id();
}
void register_init()
{
    g_algoRegister.register_init();
}
 int register_read()
{
    return g_algoRegister.register_read();
 }
 void register_write()
 {
    return g_algoRegister.register_write();
 }
 void register_reset()
 {
    g_algoRegister.register_reset();
 }
 void register_dump()
 {
    g_algoRegister.register_dump();
 }

#ifdef __cplusplus
};
#endif
