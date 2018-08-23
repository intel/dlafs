/*
 * Copyright (c) 2018 Intel Corporation
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

#ifndef _OCL_VPP_BLENDER_H_
#define _OCL_VPP_BLENDER_H_

#include "oclvppbase.h"

namespace HDDLStreamFilter
{

class OclVppBlender : public OclVppBase
{
public:
    OclStatus process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&);
    const char* getKernelFileName () { return "blend"; }
    const char* getKernelName() { return "blend"; }

private:
    OclStatus blend_helper ();

    guint32   m_dst_w;
    guint32   m_dst_h;

    guint32   m_src_w;
    guint32   m_src_h;

    OclCLMemInfo *m_src; //osd
    OclCLMemInfo *m_src2; //nv12
    OclCLMemInfo *m_dst;

    cl_mem clBuffer_osd;
    cl_mem clBuffer_dst;

    static const bool s_registered;
};

}
#endif // _OCL_VPP_BLENDER_H_
