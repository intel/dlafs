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


#include "hddlspipe.h"

int  main(int argc, char **argv)
{
    HddlsPipe*   pipe = NULL;
    hddlspipe_prepare(argc, argv);

    pipe = hddlspipe_create( );
    if(!pipe) {
        g_print("Error: failed to create pipel ine!\n");
        return 1;
    }

   // blocked until hddpspipe_stop.
   hddlspipe_start(pipe);
   // replay if need
   hddlspipes_replay_if_need(pipe);
   hddlspipe_destroy (pipe);

   return 0;
}

