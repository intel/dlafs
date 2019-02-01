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
#include <glib.h>
#include "jsonpacker.h"


JsonPackage::JsonPackage()
{
    json_root = json_object_new_object();
    if(!json_root) {
        g_print("Error: failed to create a new json object");
    }
}

JsonPackage::~JsonPackage()
{
    if(json_root)
        json_object_put(json_root);
    json_root = NULL;
}

int JsonPackage::add_object_int(const char *key, int32_t value)
{
    if(!json_root)
        return -1;
    int ret = 0;
    struct json_object* obj = NULL;
    if(json_object_object_get_ex (json_root, key, &obj)) {
        ret = json_object_set_int(obj, value);
    }else{
        obj = json_object_new_int(value);
        ret = json_object_object_add(json_root, key, obj);
    }
    return ret;
}
int JsonPackage::add_object_int64(const char *key, int64_t value)
{
    if(!json_root)
        return -1;
    int ret = 0;
    struct json_object* obj = NULL;
    if(json_object_object_get_ex (json_root, key, &obj)) {
        ret = json_object_set_int64(obj, value);
    }else{
        obj = json_object_new_int64(value);
        ret = json_object_object_add(json_root, key, obj);
    }
    return ret;
}
int JsonPackage::add_object_double(const char *key, double value)
{
    if(!json_root)
        return -1;
    int ret = 0;
    struct json_object* obj = NULL;
    if(json_object_object_get_ex (json_root, key, &obj)) {
        ret = json_object_set_double(obj, value);
    }else{
        obj = json_object_new_double(value);
        ret = json_object_object_add(json_root, key, obj);
    }
    return ret;
}
int JsonPackage::add_object_string(const char *key, const char* str)
{
    if(!json_root)
        return -1;
    int ret = 0;
    struct json_object* obj = NULL;
    if(json_object_object_get_ex (json_root, key, &obj)) {
        ret = json_object_set_string(obj, str);
    }else{
        obj = json_object_new_string(str);
        ret = json_object_object_add(json_root, key, obj);
    }
    return ret;
}

int JsonPackage::add_object_rect(const char *key,
                    int32_t x, int32_t y, int32_t w,int32_t h)
{
    if(!json_root)
        return -1;
    int ret = 0;
    struct json_object* obj = NULL;
    if(json_object_object_get_ex (json_root, key, &obj)) {
        g_print("Error: %s existed already when add json object!\n", key);
        return -1;
    }

    obj = json_object_new_object();
    if(obj){
        struct json_object* obj_x = json_object_new_int(x);
        struct json_object* obj_y = json_object_new_int(y);
        struct json_object* obj_w = json_object_new_int(w);
        struct json_object* obj_h = json_object_new_int(h);
        if(obj_x && obj_y && obj_w && obj_h) {
            json_object_object_add(obj, "x", obj_x);
            json_object_object_add(obj, "y", obj_y);
            json_object_object_add(obj, "w", obj_w);
            json_object_object_add(obj, "h", obj_h);
        }
        ret = json_object_object_add(json_root, key, obj);
    }
    return ret;
}

const char* JsonPackage::object_to_string()
{
    if(!json_root)
        return NULL;
    return json_object_to_json_string(json_root);
}
   