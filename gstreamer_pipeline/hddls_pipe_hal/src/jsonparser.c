/*Copyright (C) 2018 Intel Corporation
  * 
  *SPDX-License-Identifier: MIT
  *
  *MIT License
  *
  *Copyright (c) 2018 Intel Corporation
  *
  *Permission is hereby granted, free of charge, to any person obtaining a copy of
  *this software and associated documentation files (the "Software"),
  *to deal in the Software without restriction, including without limitation the rights to
  *use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
  *and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  *
  *The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  *
  *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  *INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
  *AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  *DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  */

#include <string.h>
#include <assert.h>
#include <json-c/json.h>
#include "jsonparser.h"

struct json_object*
json_create (const char* desc) {
    return json_tokener_parse (desc);
}

void
json_destroy (struct json_object** obj) {
    json_object_put(*obj);
    *obj = NULL;
}

 gboolean
json_get_int (struct json_object* parent, const char* name, int* value)
{
    struct json_object *object = NULL;

    if (json_object_object_get_ex (parent, name, &object) &&
        json_object_is_type (object, json_type_int)) {
        *value = json_object_get_int (object);
        return TRUE;
    }

    return FALSE;
}

gboolean
json_get_string (struct json_object* parent, const char* name,  const char** value)
{
    struct json_object *object = NULL;

    if (json_object_object_get_ex (parent, name, &object) &&
        json_object_is_type (object, json_type_string)) {
        *value = json_object_get_string (object);
        return TRUE;
    }

    return FALSE;
}

gboolean
json_get_object (struct json_object* parent, const char* name,  struct json_object **value)
{
    struct json_object *object = NULL;

    if (json_object_object_get_ex (parent, name, &object) &&
        json_object_is_type (object, json_type_object)) {
        *value =object;
        return TRUE;
    }

    return FALSE;
}

gboolean
json_get_int_d2 (struct json_object* parent, const char* name, const char *subname,  int* value)
{
         struct json_object *subobject = NULL;
         
        if(json_get_object(parent, name, &subobject)) {
                if(json_get_int(subobject, subname, value)) {
                    return TRUE;
                }
        }
        return FALSE;
}

gboolean
json_get_string_d2 (struct json_object* parent, const char* name, const char *subname,  const char** value)
{
         struct json_object *subobject = NULL;
         
        if(json_get_object(parent, name, &subobject)) {
                if(json_get_string(subobject, subname, value)) {
                    return TRUE;
                }
        }
        return FALSE;
}

 gboolean
 json_get_object_d2 (struct json_object* parent, const char* name, const char *subname,  struct json_object **value)
 {
     struct json_object *object = NULL;
 
     if (json_get_object (parent, name, &object) ) {
         if(json_get_object (object, subname, value))
                return TRUE;
     }
 
     return FALSE;
 }

 enum E_COMMAND_TYPE  json_get_command_type( json_object *root)
{
       int command_type = -1;

      if(json_get_int(root, "command_type", &command_type))
           return   (enum E_COMMAND_TYPE)command_type;
      else
          return eCommand_None;
}

