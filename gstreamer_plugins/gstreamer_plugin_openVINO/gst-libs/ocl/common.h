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

#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

#include <interface/vpphost.h>
#include <interface/videodefs.h>
#include <interface/vppinterface.h>
#include <ocl/oclvppcrc.h>
#include <ocl/oclmemory.h>
#include <ocl/oclutils.h>
#include <ocl/crcmeta.h>

#ifndef ALIGN_POW2
#define ALIGN_POW2(a, b) ((a + (b - 1)) & ~(b - 1))
#endif


#endif
