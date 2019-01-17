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
    cv::ocl::Kernel m_kernel;
    SharedPtr<OclContext> m_context;
};

}

#endif                          /* _VPP_BASE_H_ */
