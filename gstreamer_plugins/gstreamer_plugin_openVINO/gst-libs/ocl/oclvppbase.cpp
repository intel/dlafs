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

#include <va/va_drmcommon.h>
#include "oclvppbase.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace HDDLStreamFilter
{

OclVppBase::OclVppBase () : m_pixel_size(1)
{
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

    m_kernel = m_context->acquireKernelCV (getKernelName(), getKernelFileName());
    GST_LOG("kernel:%s - m_kernel = %p\n", getKernelName(), &m_kernel);
    if (m_kernel.empty()) {
        GST_ERROR("invalid kernel file: %s.cl\n", getKernelFileName());
        return OCL_FAIL;
    }
    return OCL_SUCCESS;
}

OclStatus
OclVppBase::printOclKernelInfo()
{
    return OCL_SUCCESS;
}

}
