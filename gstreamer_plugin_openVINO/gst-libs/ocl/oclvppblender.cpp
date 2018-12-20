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

#include "oclvppblender.h"
#include "common/log.h"
#include "common/macros.h"
#include "Vppfactory.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <opencv2/opencv.hpp>
#include <CL/cl.h>
#include <opencv2/core/ocl.hpp>

using namespace cv;
using namespace cv::ocl;

namespace HDDLStreamFilter
{

#ifdef USE_CV_OCL
OclStatus OclVppBlender::blend_helper ()
{
    gboolean ret;   
    m_kernel.args(m_src2->cl_memory[0], m_src2->cl_memory[1], m_src->cl_memory[0],
                  m_dst->cl_memory[0], m_dst_w, m_dst_h);

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2 (m_dst_w, 2 * localWorkSize[0]) / 2;
    globalWorkSize[1] = ALIGN_POW2 (m_dst_h, 2 * localWorkSize[1]) / 2;
    ret = m_kernel.run(2, globalWorkSize, localWorkSize, true);
    if(!ret) {
        GST_ERROR("%s() - failed to run kernel!!!\n",__func__);
        return OCL_FAIL;
    }

    return OCL_SUCCESS;
}

#else
OclStatus OclVppBlender::blend_helper ()
{
    cl_int status = CL_SUCCESS;

    status = clSetKernelArg (m_kernel, 0, sizeof(cl_mem), &m_src2->cl_memory[0]);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }
    status = clSetKernelArg (m_kernel, 1, sizeof(cl_mem), &m_src2->cl_memory[1]);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    cl_int err;
    cl_context context = m_context->getContext();
    cl_image_desc desc =
    {
        CL_MEM_OBJECT_IMAGE2D,
        m_dst_w,
        m_dst_h,
        0, 0, // depth, array size (unused)
        m_dst_w*4,
        0, 0, 0, 0
    };
    cl_image_format format;
    format.image_channel_order = CL_BGRA;
    format.image_channel_data_type = CL_UNORM_INT8;
    clBuffer_osd = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR ,&format,
                    (const cl_image_desc *)&desc, (void *)m_src->cl_memory[0], &err);
    status = clSetKernelArg (m_kernel, 2, sizeof(cl_mem), &clBuffer_osd);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    clBuffer_dst = clCreateImage(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR ,&format,
                    (const cl_image_desc *)&desc, (void *)m_dst->cl_memory[0], &err);
    status = clSetKernelArg (m_kernel, 3, sizeof(cl_mem), &clBuffer_dst);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    status = clSetKernelArg (m_kernel, 4, sizeof(guint32), &m_dst_w);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }
    status = clSetKernelArg (m_kernel, 5, sizeof(guint32), &m_dst_h);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    size_t globalWorkSize[2], localWorkSize[2];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2 (m_dst_w, 2 * localWorkSize[0]) / 2;
    globalWorkSize[1] = ALIGN_POW2 (m_dst_h, 2 * localWorkSize[1]) / 2;

    //(oclvppblender.cpp,134) clEnqueueNDRangeKernel failed: CL_INVALID_KERNEL_ARGS ????
    if (CL_ERROR_PRINT (clEnqueueNDRangeKernel (m_context->getCommandQueue(),
        m_kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL), "clEnqueueNDRangeKernel")) {
        return OCL_FAIL;
    }

    return OCL_SUCCESS;
}
#endif
OclStatus
OclVppBlender::process (const SharedPtr<VideoFrame>& src,  /* osd */
                           const SharedPtr<VideoFrame>& src2, /* NV12 */
                           const SharedPtr<VideoFrame>& dst) /* rgb output */
{
    OclStatus status = OCL_SUCCESS;

    m_src_w = src->width;
    m_src_h = src->height;
    m_dst_w  = dst->width;
    m_dst_h  = dst->height;

    clBuffer_osd = 0;
    clBuffer_dst = 0;
    if (!m_dst_w || !m_dst_h || !m_src_w || !m_src_h) {
        GST_ERROR("OclVppBlender failed due to invalid resolution\n");
        return OCL_INVALID_PARAM;
    }

    if((m_src_w != m_dst_w) || (m_src_h != m_dst_h)) {
        GST_ERROR("OclVppBlender failed due to dst and src is not same resolution!\n");
        return OCL_INVALID_PARAM;
    }

#ifdef USE_CV_OCL
    if (m_kernel.empty())
#else
    if (!m_kernel)
#endif
    {
        GST_ERROR ("OclVppBlender: invalid kernel\n");
        return OCL_FAIL;
    }

    m_src = (OclCLMemInfo*) m_context->acquireMemoryCL (src->mem, 1);
    if (!m_src) {
        GST_ERROR("failed to acquire src memory\n");
        m_context->finish();
        return OCL_FAIL;
    }

    m_src2 = (OclCLMemInfo*) m_context->acquireVAMemoryCL ((VASurfaceID*)&src2->surface, 2);
    if (!m_src2) {
        GST_ERROR("failed to acquire src2 va memory\n");
        m_context->releaseMemoryCL(&m_src);
        return OCL_FAIL;
    }

    m_dst = (OclCLMemInfo*) m_context->acquireMemoryCL (dst->mem, 1);
    if (!m_src2) {
        GST_ERROR("failed to acquire src2 memory\n");
        m_context->releaseMemoryCL(&m_src);
        m_context->releaseVAMemoryCL(&m_src2);
        m_context->finish();
        return OCL_FAIL;
    }

    printOclKernelInfo(); // test
    status = blend_helper();

    if(clBuffer_osd)
        clReleaseMemObject(clBuffer_osd);
    clBuffer_osd = 0;

    if(clBuffer_dst)
        clReleaseMemObject(clBuffer_dst);
    clBuffer_dst = 0;

    m_context->releaseMemoryCL(&m_src);
    m_context->releaseMemoryCL(&m_dst);
    m_context->releaseVAMemoryCL(&m_src2);
    m_context->finish();

    return status;
}

const bool OclVppBlender::s_registered =
    OclVppFactory::register_<OclVppBlender>(OCL_VPP_BLENDER);

}
