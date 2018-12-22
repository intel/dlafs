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

#ifndef _VPP_HOST_H_
#define _VPP_HOST_H_

#include "vppinterface.h"

using namespace HDDLStreamFilter;

#define NEW_VPP_SHARED_PTR(mimeType) \
    VppInstanceCreate (mimeType), VppInstanceDestroy

extern "C"
{
    VppInterface* VppInstanceCreate (const char* mimeType);
    void VppInstanceDestroy(VppInterface* vpp);
}

#endif                          /* _VPP_HOST_H_ */
