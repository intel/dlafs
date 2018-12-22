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

/*
  * Author: River,Li
  * Email: river.li@intel.com
  * Date: 2018.10
  */

#ifndef _OCL_VPP_CRC_H_
#define _OCL_VPP_CRC_H_

#include "oclvppbase.h"

namespace HDDLStreamFilter
{

class OclVppCrc : public OclVppBase
{

public:
    OclStatus process (const SharedPtr<VideoFrame>&, const SharedPtr<VideoFrame>&);

    const char* getKernelFileName () {
        return "crc";
    }

    const char* getKernelName() {
        switch(m_crc_format) {
            case CRC_FORMAT_BGR:
                return "crop_resize_csc";
                break;
            case CRC_FORMAT_BGR_PLANNAR:
                return "crop_resize_csc_planar";
                break;
            case CRC_FORMAT_GRAY:
                return "crop_resize_csc_gray";
                break;
            default:
                return "";
                break;
        }
        return "";
    }

    gboolean setParameters (gpointer);
    void setOclFormat(CRCFormat crc_format) {m_crc_format = crc_format;}

    explicit OclVppCrc () : m_planar(true), m_src(NULL), m_dst(NULL) {}

private:

    OclStatus crc_helper ();
    gboolean  m_planar;

    CRCFormat m_crc_format;
    guint32   m_dst_w;
    guint32   m_dst_h;

    guint32   m_src_w;
    guint32   m_src_h;
    guint32   m_crop_x;
    guint32   m_crop_y;
    guint32   m_crop_w;
    guint32   m_crop_h;

    OclCLMemInfo *m_src;
    OclCLMemInfo *m_dst;

    static const bool s_registered;
};

}

#endif //_OCL_VPP_CRC_H_
