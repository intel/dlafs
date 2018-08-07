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
//#include "ocl/oclmixmeta.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace HDDLStreamFilter
{

OclVppBase::OclVppBase () : m_meta_list(NULL), m_pixel_size(1), m_kernel(0)
{
}

OclVppBase::~OclVppBase ()
{
    destroyMetaList ();
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
OclVppBase::setOclContext (const SharedPtr<OclContext>& context)
{
    m_context = context;

    m_kernel = m_context->acquireKernel (getKernelName(), getKernelFileName());
    if (!m_kernel) {
        g_print ("invalid kernel file: %s.cl\n", getKernelFileName());
        return OCL_FAIL;
    }

    return OCL_SUCCESS;
}

OclStatus
OclVppBase::setNativeDisplay (const VADisplay, const OclNativeDisplayType)
{
    return OCL_SUCCESS;
}

GList*
OclVppBase::getMetaList ()
{
    return m_meta_list;
}

void
OclVppBase::addMetaList (GList* meta_list)
{
    checkMetaList (&meta_list);

    if (meta_list)
        m_meta_list = (m_meta_list ? g_list_concat (m_meta_list, meta_list) : meta_list);
}

void
OclVppBase::resetMetaList (GList* meta_list)
{
    if (m_meta_list) {
        destroyMetaList ();
    }

    m_meta_list = meta_list;
}

void
OclVppBase::destroyMetaList ()
{
    if (m_meta_list) {
        g_list_free_full (m_meta_list, (GDestroyNotify) NULL);
        m_meta_list = NULL;
    }
}

}
