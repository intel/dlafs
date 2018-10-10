/*
 * Copyright (c) 2017 Intel Corporation
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

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "log.h"
#include "lock.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int oclLogFlag;
FILE* oclLogFn;
int isInit = 0;
static HDDLStreamFilter::Lock g_traceLock;

void oclTraceInit()
{
    HDDLStreamFilter::AutoLock locker(g_traceLock);
    if(!isInit){
        char* liboclLogLevel = getenv("LIBOCL_LOG_LEVEL");
        char* libyamLog = getenv("LIBOCL_LOG");
        FILE* tmp;

        oclLogFn = stderr;
        if (liboclLogLevel){
            oclLogFlag = atoi(liboclLogLevel);
            if (libyamLog){
                time_t now;
                struct tm* curtime;
                char filename[80];

                time(&now);

                if ((curtime = localtime(&now))) {
                    snprintf(filename, sizeof(filename), "%s_%2d_%02d_%02d_%02d_%02d", libyamLog,
                    	curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday,
                    	curtime->tm_hour, curtime->tm_sec);
                } else {
                    snprintf(filename, sizeof(filename), "%s", libyamLog);
                }

                if ((tmp = fopen(filename, "w"))){
                    oclLogFn = tmp;
                    ERROR("Libocl_Trace is on, save log into %s.\n", filename);
                } else {
                    ERROR("Open file %s failed.\n", filename);
                }
            }
        }
        else {
            oclLogFlag = OCL_LOG_ERROR;
        }
        isInit = 1;
    }
#ifndef __ENABLE_DEBUG__
     if (oclLogFlag > OCL_LOG_ERROR)
         fprintf(stderr, "ocl log isn't enabled (--enable-debug)\n");
 #endif
}

void print_log(const char* msg, struct timespec* tmspec, struct timespec* early_time)
{
    struct timespec tp;
    struct timespec *curr_time = (tmspec) ? tmspec : &tp;

    clock_gettime(CLOCK_REALTIME, curr_time);

    struct tm *info = localtime(&curr_time->tv_sec);

    if (!info)
        return;

#ifndef DETAIL_LOG
    if (tmspec == NULL && early_time == NULL) {
#endif

    if (early_time) {
        long msec = (curr_time->tv_sec - early_time->tv_sec) * 1000 + (curr_time->tv_nsec - early_time->tv_nsec) / 1000000;
        long nsec = (curr_time->tv_nsec - early_time->tv_nsec) % 1000000;

        printf("%4d-%02d-%02d %02d:%02d:%02d.%09ld\t%s == %01ld.%ldms\n", info->tm_year + 1900,
                info->tm_mon + 1, info->tm_mday, info->tm_hour, info->tm_min,
                info->tm_sec, curr_time->tv_nsec, msg, msec, nsec);
    } else {
        printf("%4d-%02d-%02d %02d:%02d:%02d.%09ld\t%s\n", info->tm_year + 1900,
                info->tm_mon + 1, info->tm_mday, info->tm_hour, info->tm_min,
                info->tm_sec, curr_time->tv_nsec, msg);
    }

#ifndef DETAIL_LOG
    }
#endif
}
