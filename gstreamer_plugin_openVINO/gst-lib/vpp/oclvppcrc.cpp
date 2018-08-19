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

#include "oclvppcrc.h"
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

#define ROTATE_KERNEL_FILE "crc"

OclStatus
OclVppCrc::crc_helper()
{
    cl_int status = CL_SUCCESS;

#if 0
    if ((status = clSetKernelArg (m_kernel, 0, sizeof(cl_mem), &m_src->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 1, sizeof(cl_mem), &m_src->cl_memory[1])) ||
        (status = clSetKernelArg (m_kernel, 2, sizeof(guint32), &m_src_w)) ||
        (status = clSetKernelArg (m_kernel, 3, sizeof(guint32), &m_src_h)) ||
        (status = clSetKernelArg (m_kernel, 4, sizeof(guint32), &m_crop_x)) ||
        (status = clSetKernelArg (m_kernel, 5, sizeof(guint32), &m_crop_y)) ||
        (status = clSetKernelArg (m_kernel, 6, sizeof(guint32), &m_crop_w)) ||
        (status = clSetKernelArg (m_kernel, 7, sizeof(guint32), &m_crop_h)) ||
        (status = clSetKernelArg (m_kernel, 8, sizeof(cl_mem), &m_dst->cl_memory[0])) ||
        (status = clSetKernelArg (m_kernel, 9, sizeof(guint32), &m_src_w)) ||
        (status = clSetKernelArg (m_kernel, 10, sizeof(guint32), &m_src_h))) {
            CL_ERROR_PRINT (status, "clSetKernelArg");
            return OCL_FAIL;
        }
#else
status = clSetKernelArg (m_kernel, 0, sizeof(cl_mem), &m_src->cl_memory[0]);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 1, sizeof(cl_mem), &m_src->cl_memory[1]);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 2, sizeof(guint32), &m_src_w);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 3, sizeof(guint32), &m_src_h);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 4, sizeof(guint32), &m_crop_x);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 5, sizeof(guint32), &m_crop_y);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 6, sizeof(guint32), &m_crop_w);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 7, sizeof(guint32), &m_crop_h);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}

status = clSetKernelArg (m_kernel, 8, sizeof(cl_mem), &m_dst->cl_memory[0]);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 9, sizeof(guint32), &m_src_w);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
status = clSetKernelArg (m_kernel, 10, sizeof(guint32), &m_src_h);
if(status != CL_SUCCESS){
    CL_ERROR_PRINT (status, "clSetKernelArg");
    return OCL_FAIL;
}
#endif

    size_t globalWorkSize[2], localWorkSize[2];

    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    globalWorkSize[0] = ALIGN_POW2 (m_crop_w, 2 * localWorkSize[0]) / 2;
    globalWorkSize[1] = ALIGN_POW2 (m_crop_h, 2 * localWorkSize[1]) / 2;

    if (CL_ERROR_PRINT (clEnqueueNDRangeKernel (m_context->getCommandQueue(), m_kernel, 2, NULL,
                               globalWorkSize, localWorkSize, 0, NULL, NULL), "clEnqueueNDRangeKernel")) {
        return OCL_FAIL;
    }

    return OCL_SUCCESS;
}

OclStatus
OclVppCrc::process (const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dst)
{
    OclStatus status = OCL_SUCCESS;

    m_src_w = src->width;
    m_src_h = src->height;
    m_crop_x = src->crop.x;
    m_crop_x = src->crop.y;
    m_crop_w = src->crop.width;
    m_crop_h = src->crop.height;
    m_dst_w  = dst->width;
    m_dst_h  = dst->height;

    if (!m_dst_w || !m_dst_h || !m_src_w || !m_src_h || !m_crop_w || !m_crop_h) {
        g_print ("OclVppCrc failed due to invalid resolution\n");
        return OCL_INVALID_PARAM;
    }

    if (!m_kernel) {
        g_print ("OclVppCrc: invalid kernel\n");
        return OCL_FAIL;
    }

    m_src = (OclCLMemInfo*) m_context->acquireVAMemoryCL ((VASurfaceID*)&src->surface, 2);
    if (!m_src) {
        g_print ("failed to acquire src va memory\n");
        return OCL_FAIL;
    }

    m_dst = (OclCLMemInfo*) m_context->acquireMemoryCL (dst->mem, 1);
    if (!m_dst) {
        g_print ("failed to acquire dst va memory\n");
        m_context->releaseVAMemoryCL(&m_src);
        m_context->finish();
        return OCL_FAIL;
    }

    printOclKernelInfo(); // test
    status = crc_helper();

    m_context->releaseMemoryCL(&m_dst);
    m_context->releaseVAMemoryCL(&m_src);
    m_context->finish();

    return status;
}

gboolean
OclVppCrc::setParameters(gpointer data)
{
    VppCrcParam *param = (VppCrcParam*) data;
    if (param && param->type == VPP_CRC_PARAM) {
        m_crop_x = param->crop_x;
        m_crop_y = param->crop_y;
        m_crop_w = param->crop_w;
        m_crop_h = param->crop_h;
        m_src_w  = param->src_w;
        m_src_h  = param->src_h;
        m_dst_w  = param->dst_w;
        m_dst_h  = param->dst_h;
        return TRUE;
    }
    return FALSE;
}

const bool OclVppCrc::s_registered =
    OclVppFactory::register_<OclVppCrc>(OCL_VPP_CRC);

}
