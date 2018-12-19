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

using namespace std;
#include <vector>
#include <string>

//#ifdef __cplusplus
//extern "C" {
//#endif

enum {
    ALGO_NONE      = -1,
    ALGO_YOLOV1_TINY           = 0,
    ALGO_OF_TRACK                  = 1,
    ALGO_GOOGLENETV2         = 2,
    ALGO_MOBILENET_SSD     = 3,
    ALGO_TRACK_LP                  =4,
    ALGO_LPRNET                        = 5,
    ALGO_YOLOV2_TINY           = 6,
    ALGO_REID                               = 7,
    ALGO_SINK,   /*last algo in algopipe*/
    ALGO_MAX_DEFAULT_NUM,
};

#define ALGO_YOLOV1_TINY_NAME "yolov1tiny"
#define ALGO_OF_TRACK_NAME "opticalflowtrack"
#define ALGO_GOOGLENETV2_NAME "googlenetv2"
#define ALGO_MOBILENET_SSD_NAME "mobilenetssd"
#define ALGO_TRACK_LP_NAME "tracklp"
#define ALGO_LPRNET_NAME "lprnet"
#define ALGO_YOLOV2_TINY_NAME "yolov2tiny"
#define ALGO_REID_NAME "reid"
#define ALGO_GENERIC_DL_NAME "generic"
#define ALGO_SINK_NAME "sink"

#define ALGO_REGISTER_FILE_NAME  ".algocache.register"
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

//#ifdef __cplusplus
//};
//#endif

#endif
