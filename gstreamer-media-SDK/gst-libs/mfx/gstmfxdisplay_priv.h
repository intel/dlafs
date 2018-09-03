/*
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2013 Intel Corporation
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

#ifndef GST_MFX_DISPLAY_PRIV_H
#define GST_MFX_DISPLAY_PRIV_H

#include "gstmfxdisplay.h"
#include "gstmfxwindow.h"
#include "gstmfxwindow_priv.h"
#include "gstmfxminiobject.h"

G_BEGIN_DECLS

#define GST_MFX_DISPLAY_CAST(display) \
  ((GstMfxDisplay *) (display))

#define GST_MFX_DISPLAY_GET_PRIVATE(display) \
  (&GST_MFX_DISPLAY_CAST (display)->priv)

#define GST_MFX_DISPLAY_CLASS(klass) \
  ((GstMfxDisplayClass *) (klass))

#define GST_MFX_IS_DISPLAY_CLASS(klass) \
  ((klass) != NULL)

#define GST_MFX_DISPLAY_GET_CLASS(obj) \
  GST_MFX_DISPLAY_CLASS (GST_MFX_MINI_OBJECT_GET_CLASS (obj))

typedef struct _GstMfxDisplayPrivate          GstMfxDisplayPrivate;
typedef struct _GstMfxDisplayClass            GstMfxDisplayClass;
typedef enum _GstMfxDisplayInitType           GstMfxDisplayInitType;

typedef void(*GstMfxDisplayInitFunc) (GstMfxDisplay * display);
typedef gboolean(*GstMfxDisplayBindFunc) (GstMfxDisplay * display,
  gpointer native_dpy);
typedef gboolean(*GstMfxDisplayOpenFunc) (GstMfxDisplay * display,
  const gchar * name);
typedef void(*GstMfxDisplayCloseFunc) (GstMfxDisplay * display);
typedef void(*GstMfxDisplayGetSizeFunc) (GstMfxDisplay * display,
  guint * pwidth, guint * pheight);
typedef void(*GstMfxDisplayGetSizeMFunc) (GstMfxDisplay * display,
  guint * pwidth, guint * pheight);
typedef GstMfxWindow *(*GstMfxDisplayCreateWindowFunc) (
  GstMfxDisplay * display, GstMfxID id, guint width, guint height);

/**
* GST_MFX_DISPLAY_GET_CLASS_TYPE:
* @display: a #GstMfxDisplay
*
* Returns the #display class type
* This is an internal macro that does not do any run-time type check.
*/
#undef  GST_MFX_DISPLAY_GET_CLASS_TYPE
#define GST_MFX_DISPLAY_GET_CLASS_TYPE(display) \
  (GST_MFX_DISPLAY_GET_CLASS (display)->display_type)

/**
* GST_MFX_DISPLAY_HANDLE:
* @display: a #GstMfxDisplay
*
* Macro that evaluates to the native display of @display.
* This is an internal macro that does not do any run-time type check.
*/
#undef  GST_MFX_DISPLAY_HANDLE
#define GST_MFX_DISPLAY_HANDLE(display) \
  (GST_MFX_DISPLAY_GET_PRIVATE (display)->native_display)


struct _GstMfxDisplayPrivate
{
  GRecMutex mutex;
  GstMfxDisplayType display_type;
  int display_fd;
  VADisplay va_display;
  gpointer native_display;
  guint width;
  guint height;
  guint width_mm;
  guint height_mm;
  guint par_n;
  guint par_d;
  gchar *vendor_string;
  gboolean is_opengl;
};

/**
 * GstMfxDisplay:
 *
 * Base class for VA displays.
 */
struct _GstMfxDisplay
{
  /*< private >*/
  GstMfxMiniObject parent_instance;

  GstMfxDisplayPrivate priv;
};

/**
 * GstMfxDisplayClass:
 * @open_display: virtual function to open a display
 * @close_display: virtual function to close a display
 * @get_size: virtual function to retrieve the display dimensions, in pixels
 * @get_size_mm: virtual function to retrieve the display dimensions, in millimeters
 * @create_window: (optional) virtual function to create a window
 *
 * Base class for VA displays.
 */
struct _GstMfxDisplayClass
{
  /*< private >*/
  GstMfxMiniObjectClass parent_class;

  /*< protected >*/
  guint display_type;

  /*< public >*/
  GstMfxDisplayInitFunc init;
  GstMfxDisplayBindFunc bind_display;
  GstMfxDisplayOpenFunc open_display;
  GstMfxDisplayCloseFunc close_display;
  GstMfxDisplayGetSizeFunc get_size;
  GstMfxDisplayGetSizeMFunc get_size_mm;
  GstMfxDisplayCreateWindowFunc create_window;
};

/* Initialization types */
enum _GstMfxDisplayInitType
{
  GST_MFX_DISPLAY_INIT_FROM_DISPLAY_NAME = 1,
  GST_MFX_DISPLAY_INIT_FROM_NATIVE_DISPLAY
};

void
gst_mfx_display_class_init(GstMfxDisplayClass * klass);

GstMfxDisplay *
gst_mfx_display_new_internal (const GstMfxDisplayClass * klass,
    GstMfxDisplayInitType init_type, gpointer init_value);

#define gst_mfx_display_ref_internal(display) \
  ((gpointer)gst_mfx_mini_object_ref(GST_MFX_MINI_OBJECT(display)))

#define gst_mfx_display_unref_internal(display) \
  gst_mfx_mini_object_unref(GST_MFX_MINI_OBJECT(display))

#define gst_mfx_display_replace_internal(old_display_ptr, new_display) \
  gst_mfx_mini_object_replace((GstMfxMiniObject **)(old_display_ptr), \
  GST_MFX_MINI_OBJECT(new_display))

#undef  gst_mfx_display_ref
#define gst_mfx_display_ref(display) \
  gst_mfx_display_ref_internal((display))

#undef  gst_mfx_display_unref
#define gst_mfx_display_unref(display) \
  gst_mfx_display_unref_internal((display))

#undef  gst_mfx_display_replace
#define gst_mfx_display_replace(old_display_ptr, new_display) \
  gst_mfx_display_replace_internal((old_display_ptr), (new_display))

G_END_DECLS

#endif /* GST_MFX_DISPLAY_PRIV_H */
