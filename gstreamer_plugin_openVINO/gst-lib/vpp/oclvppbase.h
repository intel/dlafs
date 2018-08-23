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

#ifndef _VPP_BASE_H_
#define _VPP_BASE_H_

#include <va/va.h>
#include <CL/opencl.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/va_intel.hpp>

#include <interface/vppinterface.h>
#include <ocl/oclcommon.h>

namespace HDDLStreamFilter
{

class OclVppBase : public VppInterface
{
public:
    OclVppBase ();
    virtual ~OclVppBase ();

    virtual OclStatus
    process ();

    virtual OclStatus
    process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&);

    virtual OclStatus
    process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&);

    virtual gboolean
    setParameters (gpointer) { return FALSE; }

    virtual gboolean
    getParameters (gpointer) { return FALSE; }

    virtual OclStatus
    setOclContext (const SharedPtr<OclContext>&);

    virtual OclStatus
    setNativeDisplay (const VADisplay, const OclNativeDisplayType);

    virtual const char*
    getKernelFileName () { return NULL; }

    virtual const char*
    getKernelName() { return NULL; }

    virtual void
    setOclFormat(CRCFormat crc_format) {}

    OclStatus
    printOclKernelInfo();
protected:
    guint16   m_pixel_size;
    #ifdef USE_CV_OCL
    cv::ocl::Kernel m_kernel;
    #else
    cl_kernel m_kernel;
    #endif
    SharedPtr<OclContext> m_context;
};

}

#endif                          /* _VPP_BASE_H_ */
