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

  /*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

using namespace std;
#include <string.h>
#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <getopt.h>

#include <algoregister.h>

 static void print_usage (const char* program_name, gint exit_code)
{
    g_print ("Usage: %s...\n", program_name);
    g_print (
        " -l --list all algo in register.\n"
        " -a --add a new algo into register.\n"
        " -c -- clear all algo cache.\n"
        " -h --help Display this usage information.\n");
    exit (exit_code);
  }

int main(int argc, char **argv)
{
     const char* const brief = "ha:lc";
     const struct option details[] = {
                { "list", 1, NULL, 'l'},
                { "add", 1, NULL, 'a'},
                { "clear",1,NULL,'c'},
                { "help", 0, NULL, 'h'},
                { NULL, 0, NULL, 0 }
       };
    int opt = 0;
    int id = 0;

    while (opt != -1) {
        opt = getopt_long (argc, argv, brief, details, NULL);
        switch (opt) {
            case 'l':
                register_init();
                register_dump();
                break;
            case 'a':
                register_init();
                 id = register_get_free_algo_id();
                 g_print("register: id=%d, name=%s\n",id, optarg);
                register_add_algo(id, optarg);
                register_write();
                register_dump();
                break;
            case 'c':
                register_reset();
                //register_write();
                register_dump();
                break;
            case 'h': /* help */
                print_usage (argv[0], 0);
                break;
            case '?': /* an invalid option. */
                print_usage (argv[0], 1);
                break;
            case -1: /* Done with options. */
                break;
             default: /* unexpected. */
                print_usage (argv[0], 1);
                abort ();
         }
     }
     return TRUE;    

}
