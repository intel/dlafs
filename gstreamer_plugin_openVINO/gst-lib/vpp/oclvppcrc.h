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

#ifndef _OCL_VPP_CRC_H_
#define _OCL_VPP_CRC_H_

//#include <vector>
#include "oclvppbase.h"

//using std::vector;

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
