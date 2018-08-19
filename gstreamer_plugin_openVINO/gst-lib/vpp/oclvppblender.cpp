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

#include <opencv2/opencv.hpp>
#include <CL/cl.h>
using namespace cv;

namespace HDDLStreamFilter
{

OclStatus
OclVppBlender::blend_helper ()
{
    cl_int status = CL_SUCCESS;
 #if 0
    if ((status = clSetKernelArg (m_kernel, 0, sizeof(cl_mem), &m_dst->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 1, sizeof(cl_mem), &m_dst->cl_memory[1])) ||
        (status = clSetKernelArg (m_kernel, 2, sizeof(cl_mem), &m_src->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 3, sizeof(cl_mem), &m_dst->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 4, sizeof(guint32), &m_dst_w)) ||
        (status = clSetKernelArg (m_kernel, 5, sizeof(guint32), &m_dst_h))) {
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }
#else
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

    // TODO: why clSetKernelArg will fail if set it directly by m_src->cl_memory[0] ?
    //         also it will fail if create UMat.

    #if 1
    //cv::UMat frame;
    //frame.create(cv::Size(m_dst_w, m_dst_h), CV_8UC3);
    //cl_mem clBuffer = (cl_mem)frame.handle(ACCESS_RW);

    cl_image_format format;
    format.image_channel_order = CL_RGBA; // single channel
    format.image_channel_data_type = CL_UNORM_INT8; // float data type

    void *temp = (void *)m_src->cl_memory[0];//(char *)malloc(m_dst_w * m_dst_h * 4);
    cl_int err;
    cl_context context = m_context->getContext();
    clBuffer_osd = clCreateImage2D(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR ,
                    &format, m_dst_w, m_dst_h, 0, temp, &err);
    status = clSetKernelArg (m_kernel, 2, sizeof(cl_mem), &clBuffer_osd);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    temp = (void *)m_dst->cl_memory[0];
    clBuffer_dst = clCreateImage2D(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR ,
                    &format, m_dst_w, m_dst_h, 0, temp, &err);
    status = clSetKernelArg (m_kernel, 3, sizeof(cl_mem), &clBuffer_dst);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }


    #else
    status = clSetKernelArg (m_kernel, 2, sizeof(cl_mem), &m_src->cl_memory[0]);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }

    status = clSetKernelArg (m_kernel, 3, sizeof(cl_mem), &m_dst->cl_memory[0]);
    if(status != CL_SUCCESS){
        CL_ERROR_PRINT (status, "clSetKernelArg");
        return OCL_FAIL;
    }
    #endif

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
#endif
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
        g_print ("OclVppBlender failed due to invalid resolution\n");
        return OCL_INVALID_PARAM;
    }

    if((m_src_w != m_dst_w) || (m_src_h != m_dst_h)) {
        g_print ("OclVppBlender failed due to dst and src is not same resolution!\n");
        return OCL_INVALID_PARAM;
    }

    if (!m_kernel) {
        g_print ("OclVppBlender: invalid kernel\n");
        return OCL_FAIL;
    }

    m_src = (OclCLMemInfo*) m_context->acquireMemoryCL (src->mem, 1);
    if (!m_src) {
        g_print ("failed to acquire src memory\n");
        m_context->finish();
        return OCL_FAIL;
    }

    m_src2 = (OclCLMemInfo*) m_context->acquireVAMemoryCL ((VASurfaceID*)&src2->surface, 2);
    if (!m_src2) {
        g_print ("failed to acquire src2 va memory\n");
        m_context->releaseMemoryCL(&m_src);
        return OCL_FAIL;
    }

    m_dst = (OclCLMemInfo*) m_context->acquireMemoryCL (dst->mem, 1);
    if (!m_src2) {
        g_print ("failed to acquire src2 memory\n");
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
