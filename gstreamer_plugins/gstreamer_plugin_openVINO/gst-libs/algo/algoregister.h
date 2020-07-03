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


#ifndef __CVDL_ALGO_REGISTER_H__
#define __CVDL_ALGO_REGISTER_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ALGO_NONE           = -1,
    ALGO_YOLOV1_TINY    = 0,
    ALGO_OF_TRACK       = 1,
    ALGO_GOOGLENETV2    = 2,
    ALGO_MOBILENET_SSD  = 3,
    ALGO_TRACK_LP       = 4,
    ALGO_LPRNET         = 5,
    ALGO_YOLOV2_TINY    = 6,
    ALGO_REID           = 7,
	ALGO_OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106 = 8,
	ALGO_OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039 = 9,
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
#define ALGO_OMZ_VEHICLE_LICENSE_PLATE_DETECTION_BARRIER_0106_NAME "vehicle-license-plate-detection-barrier-0106"
#define ALGO_OMZ_VEHICLE_ATTRIBUTES_RECOGNITION_BARRIER_0039_NAME "vehicle-attributes-recognition-barrier-0039"
#define ALGO_GENERIC_DL_NAME "generic"
#define ALGO_SINK_NAME "sink"

#define ALGO_REGISTER_FILE_NAME  ".algocache.register"

void register_add_algo(int id, const char* name);
const char *register_get_algo_name(int id);
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
