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

#ifndef _OCL_CONTEXT_H_
#define _OCL_CONTEXT_H_

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <map>
#include <glib.h>
#include <string>
#include <va/va.h>
#include <CL/cl.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/va_intel.hpp>

#include "common/lock.h"
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
    static SharedPtr<OclContext> create (VADisplay display);

    cl_context getContext ();
    cl_command_queue getCommandQueue ();
    cl_kernel acquireKernel (const char* name, const char* file = NULL);
    cv::ocl::Kernel acquireKernelCV (const char* name, const char* file = NULL);

    gpointer acquireVAMemoryCL (VASurfaceID* surface, const cl_uint num_planes,
                                        const cl_mem_flags flags = CL_MEM_READ_ONLY);
    gpointer acquireMemoryCL (cl_mem mem, const cl_uint num_planes);
    void releaseVAMemoryCL (gpointer info);
    void releaseMemoryCL (gpointer info);

    gboolean setDestSurface (VASurfaceID* surface);
    void finish ();

    ~OclContext ();

    OclCLMemInfo *m_dest_mem;
private:
    OclContext ();
    gboolean init (VADisplay display);
    SharedPtr<OclDevice> m_device;
    DISALLOW_COPY_AND_ASSIGN (OclContext)
};
}

const char* getCLErrorString (cl_int error);

gboolean checkCLError (cl_int status, const char* func, const char* file, const int line);

#endif //_OCL_CONTEXT_H_
