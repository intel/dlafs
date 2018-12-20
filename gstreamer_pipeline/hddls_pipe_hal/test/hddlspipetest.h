/*Copyright (C) 2018 Intel Corporation
  * 
  *SPDX-License-Identifier: MIT
  *
  *MIT License
  *
  *Copyright (c) <year> <copyright holders>
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

  /*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

#ifndef __HDDLS_PIPE_H__
#define __HDDLS_PIPE_H__

#include <string.h>
#include <assert.h>
#include <glib.h>
#include <gst/gst.h>
#include <wsclient.h>
#include <stdlib.h>
#include <getopt.h>

enum E_PIPE_STATE {
    ePipeState_Null = 0,
    ePipeState_Ready = 1,
    ePipeState_Running = 2,
    ePipeState_Pause = 3,
 };

enum E_CODEC_TYPE{
    eCodecTypeNone=-1,
    eCodecTypeH264 = 0,
    eCodecTypeH265 = 1,
};

typedef struct _HddlsPipe {
    enum E_PIPE_STATE      state;
    GMainLoop           *loop;
    GstElement          *pipeline;
    guint               bus_watch_id;
    struct json_object  *config;
    gint pipe_id;
}HddlsPipe;

void hddlspipe_prepare(int argc, char **argv);
HddlsPipe*   hddlspipe_create( );
void  hddlspipe_start(HddlsPipe * hp);
void  hddlspipe_stop(HddlsPipe * hp);
void hddlspipe_resume(HddlsPipe *pipe);
void hddlspipe_pause(HddlsPipe *pipe);
void hddlspipe_destroy(HddlsPipe *pipe);
 void hddlspipes_replay_if_need(HddlsPipe *pipe);
#endif
