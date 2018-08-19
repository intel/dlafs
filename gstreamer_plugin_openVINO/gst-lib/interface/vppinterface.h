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

#ifndef _VPP_INTERFACE_H_
#define _VPP_INTERFACE_H_

#include "videodefs.h"
#include "vppdefs.h"
#include "ocl/oclcontext.h"

namespace HDDLStreamFilter
{

class VppInterface
{
  public:

    virtual OclStatus
    process () = 0;

    virtual OclStatus
    process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&) = 0;

    virtual OclStatus
    process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&) = 0;

    virtual gboolean
    setParameters (gpointer) = 0;

    virtual gboolean
    getParameters (gpointer) = 0;

    virtual OclStatus
    setOclContext (const SharedPtr<OclContext>&) = 0;

    virtual OclStatus
    setNativeDisplay (const VADisplay, const OclNativeDisplayType) = 0;

    virtual void
    setOclFormat(CRCFormat crc_format) = 0;

    virtual GList*
    getMetaList () = 0;

    virtual void
    addMetaList (GList*) = 0;

    virtual void
    resetMetaList (GList*) = 0;

    virtual
    ~VppInterface () {}
};
}
#endif                          /* _VPP_INTERFACE_H_ */
