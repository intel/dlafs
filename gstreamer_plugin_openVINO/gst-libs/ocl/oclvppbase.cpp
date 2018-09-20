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

#include <va/va_drmcommon.h>

#include "oclvppbase.h"
#include "common/log.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace HDDLStreamFilter
{

OclVppBase::OclVppBase () : m_pixel_size(1)
{
#ifdef USE_CV_OCL
#else
    m_kernel = 0;
#endif
}

OclVppBase::~OclVppBase ()
{
    m_context.reset();
}

OclStatus
OclVppBase::process ()
{
    return OCL_SUCCESS;
}

OclStatus
OclVppBase::process (const SharedPtr<VideoFrame>&,const SharedPtr<VideoFrame>&)
{
    return OCL_SUCCESS;
}

OclStatus
OclVppBase::process (const SharedPtr<VideoFrame>&,const SharedPtr<VideoFrame>&,
                       const SharedPtr<VideoFrame>&)
{
    return OCL_SUCCESS;
}


OclStatus
OclVppBase::setOclContext (const SharedPtr<OclContext>& context)
{
    m_context = context;

#ifdef USE_CV_OCL
    m_kernel = m_context->acquireKernelCV (getKernelName(), getKernelFileName());
    GST_LOG("kernel:%s - m_kernel = %p\n", getKernelName(), &m_kernel);
    if (m_kernel.empty()) {
        g_print("invalid kernel file: %s.cl\n", getKernelFileName());
        return OCL_FAIL;
    }
#else
    m_kernel = m_context->acquireKernel (getKernelName(), getKernelFileName());
    GST_LOG("kernel:%s - m_kernel = %p\n", getKernelName(), m_kernel);
    if (!m_kernel) {
        GST_ERROR("invalid kernel file: %s.cl\n", getKernelFileName());
        return OCL_FAIL;
    }
#endif
    return OCL_SUCCESS;
}

OclStatus
OclVppBase::printOclKernelInfo()
{
#ifdef USE_CV_OCL
#else
    cl_int status = CL_SUCCESS;
    char kernel_name[128];
    cl_context ctx;
    cl_uint i;
    if(m_kernel) {
        status = clGetKernelInfo (m_kernel, CL_KERNEL_FUNCTION_NAME,
                 sizeof(kernel_name), kernel_name, NULL);
        if(status == CL_SUCCESS) {
            g_print("m_kernel = %p, name = %s\n", m_kernel, kernel_name);
        } else {
            g_print("printOclKernelInfo name failed = %d\n", status);
        }

        status = clGetKernelInfo (m_kernel, CL_KERNEL_CONTEXT,
            sizeof(cl_context), &ctx, NULL);
        if(status == CL_SUCCESS) {
            g_print("m_kernel = %p, context = %p\n", m_kernel, ctx);
        } else {
            g_print("printOclKernelInfo context failed = %d\n", status);
        }

        cl_uint num = 0;
        status = clGetKernelInfo (m_kernel, CL_KERNEL_NUM_ARGS,
            sizeof(cl_uint), &num, NULL);
        if(status == CL_SUCCESS) {
            g_print("m_kernel = %p, arg_num = %d\n", m_kernel, num);
        } else {
            g_print("printOclKernelInfo arg num failed = %d\n", status);
        }

        char arg_name[128];
        for(i=0;i<num;i++){
            cl_kernel_arg_access_qualifier arg_access;
            status = clGetKernelArgInfo (m_kernel, i, CL_KERNEL_ARG_ACCESS_QUALIFIER,
                     sizeof(arg_access), &arg_access, NULL);
            status = clGetKernelArgInfo (m_kernel, i, CL_KERNEL_ARG_TYPE_NAME,
                     sizeof(arg_name), &arg_name, NULL);
            cl_kernel_arg_address_qualifier arg_adress;
            status = clGetKernelArgInfo (m_kernel, i, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
                     sizeof(arg_adress), &arg_adress, NULL);
            g_print("Arg %d - access = %x, arg_type_name = %s, arg_adress = %x\n",
                    i, arg_access, arg_name, arg_adress); 
        }
    }
#endif
    return OCL_SUCCESS;
}


OclStatus
OclVppBase::setNativeDisplay (const VADisplay, const OclNativeDisplayType)
{
    return OCL_SUCCESS;
}

}
