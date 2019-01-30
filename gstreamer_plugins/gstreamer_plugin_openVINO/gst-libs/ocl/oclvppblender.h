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

#ifndef _OCL_VPP_BLENDER_H_
#define _OCL_VPP_BLENDER_H_

#include "oclvppbase.h"
#include "Vppfactory.h"

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
