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

#ifndef __JSON_PACKER_H__
#define __JSON_PACKER_H__
#include <json-c/json.h>

class JsonPackage{
public:
    JsonPackage();
    ~JsonPackage();

    int add_object_int(const char *key, int32_t value);
    int add_object_int64(const char *key, int64_t value);
    int add_object_double(const char *key, double value);
    int add_object_string(const char *key, const char* str);
    int add_object_rect(const char *key,
                                      int32_t x, int32_t y,
                                      int32_t w,int32_t h);
    const char* object_to_string();

private:
    struct json_object *json_root;
 };

#endif