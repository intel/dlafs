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

#ifndef _OCL_CONTEXT_H_
#define _OCL_CONTEXT_H_

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <map>
#include <glib.h>
#include <string>
#include <CL/cl.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>


#include "interface/lock.h"
#include "interface/videodefs.h"

typedef struct {
    cl_mem       cl_memory[3];
    cl_uint      num_planes;
} OclCLMemInfo;

#define OCL_CL_MEM_CAST(info) ((OclCLMemInfo*)info)
#define CL_ERROR_PRINT(error, func) checkCLError (error, func, __FILE__, __LINE__)

namespace HDDLStreamFilter
{

class OclDevice;

typedef std::map<std::string, cl_kernel> OclKernelMap;
typedef OclKernelMap::iterator OclKernelMapIterator;

typedef std::map<std::string, cv::ocl::Kernel> OclKernelCVMap;
typedef OclKernelCVMap::iterator OclKernelCVMapIterator;

typedef std::map<std::string, cv::ocl::Program> OclProgramCVMap;
typedef OclProgramCVMap::iterator OclProgramCVMapIterator;


class OclContext
{
public:
    static SharedPtr<OclContext> create (VideoDisplayID display);

    cl_context getContext ();
    cl_command_queue getCommandQueue ();
    //cl_kernel acquireKernel (const char* name, const char* file = NULL);
    cv::ocl::Kernel acquireKernelCV (const char* name, const char* file = NULL);

    gpointer acquireVAMemoryCL (VideoSurfaceID* surface, const cl_uint num_planes,
                                        const cl_mem_flags flags = CL_MEM_READ_ONLY);
    gpointer acquireMemoryCL (cl_mem mem, const cl_uint num_planes);
    void releaseVAMemoryCL (gpointer info);
    void releaseMemoryCL (gpointer info);

    gboolean setDestSurface (VideoSurfaceID* surface);
    void finish ();

    ~OclContext ();

    OclCLMemInfo *m_dest_mem;
private:
    OclContext ();
    gboolean init (VideoDisplayID display);
    SharedPtr<OclDevice> m_device;
    DISALLOW_COPY_AND_ASSIGN (OclContext)
};
}

const char* getErrorString (cl_int error);

gboolean checkCLError (cl_int status, const char* func, const char* file, const int line);

#endif //_OCL_CONTEXT_H_
