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

#include "vpphost.h"
#include "Vppfactory.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace HDDLStreamFilter;

extern "C" {

VppInterface*
VppInstanceCreate (const char *mimeType)
{
    if (!mimeType) {
        g_print ("Null  mime type.\n");
        return NULL;
    }

   VppInterface* instance = OclVppFactory::create (mimeType);

    if (!instance) {
        g_print ("Failed to create vpp for mimeType: '%s'\n", mimeType);
    } else {
        //g_print ("Created vpp for mimeType: '%s\n'", mimeType);
    }

    return instance;
}

void
VppInstanceDestroy (VppInterface *instance)
{
    delete instance;
}

} // extern "C"
