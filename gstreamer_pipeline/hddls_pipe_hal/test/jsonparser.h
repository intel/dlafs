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
 #ifndef __JSON_PARSER_H__
 #define __JSON_PARSER_H__

 #include <gst/gst.h>

 enum E_COMMAND_TYPE {
     eCommand_None = -1,
     eCommand_PipeCreate = 0,
     eCommand_PipeDestroy = 1,
     eCommand_SetProperty = 2,
 };

struct json_object* json_create (const char* desc) ;
void json_destroy (struct json_object** obj);
gboolean json_get_int (struct json_object* parent, const char* name, int* value);
gboolean json_get_string (struct json_object* parent, const char* name,  const char** value);
gboolean json_get_object (struct json_object* parent, const char* name,  struct json_object ** value);
gboolean json_get_int_d2 (struct json_object* parent, const char* name, const char *subname,  int* value);
gboolean json_get_string_d2 (struct json_object* parent, const char* name, const char *subname,  const char** value);
gboolean json_get_object_d2 (struct json_object* parent, const char* name, const char *subname,  struct json_object ** value);
enum E_COMMAND_TYPE  json_get_command_type( json_object *root);

#endif
