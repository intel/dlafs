/*
 *  Copyright (C) 2012-2013 Intel Corporation
 *    Author: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef GST_MFX_DISPLAY_WAYLAND_H
#define GST_MFX_DISPLAY_WAYLAND_H

#include <wayland-client.h>
#include "gstmfxdisplay.h"

G_BEGIN_DECLS

#define GST_MFX_DISPLAY_WAYLAND(obj) \
  ((GstMfxDisplayWayland *)(obj))

typedef struct _GstMfxDisplayWayland          GstMfxDisplayWayland;

GstMfxDisplay *
gst_mfx_display_wayland_new (const gchar * display_name);

G_END_DECLS

#endif /* GST_MFX_DISPLAY_WAYLAND_H */
