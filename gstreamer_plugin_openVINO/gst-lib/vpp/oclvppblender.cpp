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

#include "oclvppblender.h"
#include "common/log.h"
#include "common/macros.h"
#include "Vppfactory.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace HDDLStreamFilter
{

OclStatus
OclVppBlender::blend_helper ()
{
    cl_int status = CL_SUCCESS;
    if ((status = clSetKernelArg (m_kernel, 0, sizeof(cl_mem), &m_dst->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 1, sizeof(cl_mem), &m_dst->cl_memory[1])) ||
        (status = clSetKernelArg (m_kernel, 2, sizeof(cl_mem), &m_src->cl_memory[0]))) {
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2 (m_dst_w, 2 * localWorkSize[0]) / 2;
    globalWorkSize[1] = ALIGN_POW2 (m_dst_h, 2 * localWorkSize[1]) / 2;

    if (CL_ERROR_PRINT (clEnqueueNDRangeKernel (m_context->getCommandQueue(),
        m_kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL), "clEnqueueNDRangeKernel")) {
        return OCL_FAIL;
    }

	  return OCL_SUCCESS;
}

OclStatus
OclVppBlender::process (const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dst)
{
    OclStatus status = OCL_SUCCESS;

    if (!m_dst_w || !m_dst_h || !m_src_w || !m_src_h) {
        g_print ("crc transform failed due to invalid resolution\n");
        return OCL_INVALID_PARAM;
    }

    if (!m_kernel) {
        g_print ("OclVppBlender: invalid kernel\n");
        return OCL_FAIL;
    }

    m_dst = (OclCLMemInfo*) m_context->acquireVAMemoryCL ((VASurfaceID*)&dst->surface, 2);
    if (!m_dst) {
        g_print ("failed to acquire dst va memory\n");
        return OCL_FAIL;
    }

    m_src = (OclCLMemInfo*) m_context->acquireMemoryCL (src->mem, 1);
    if (!m_src) {
        g_print ("failed to acquire dst va memory\n");
        m_context->releaseVAMemoryCL(&m_dst);
        m_context->finish();
        return OCL_FAIL;
    }

    status = blend_helper();

    m_context->releaseMemoryCL(&m_src);
    m_context->releaseVAMemoryCL(&m_dst);
    m_context->finish();

    return status;
}

const bool OclVppBlender::s_registered =
    OclVppFactory::register_<OclVppBlender>(OCL_VPP_BLENDER);

}
