/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstwssink.h:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_WS_SINK_H__
#define __GST_WS_SINK_H__

#include <gst/gst.h>
#include <ws/wsclient.h>

G_BEGIN_DECLS


#define GST_TYPE_WS_SINK              (gst_ws_sink_get_type())
#define GST_WS_SINK(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WS_SINK,GstWsSink))
#define GST_WS_SINK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WS_SINK,GstWsSinkClass))
#define GST_WS_SINK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_WS_SINK, GstWsSinkClass))
#define GST_IS_WS_SINK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WS_SINK))
#define GST_IS_WS_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WS_SINK))
#define GST_WS_SINK_CAST(obj)         ((GstWsSink *) (obj))

/**
 * GST_WS_SINK_PAD:
 * @obj: ws sink instance
 *
 * Gives the pointer to the #GstPad object of the element.
 */
#define GST_WS_SINK_PAD(obj)          (GST_WS_SINK_CAST (obj)->sinkpad)

#define GST_WS_SINK_GET_LOCK(obj)   (&GST_WS_SINK_CAST(obj)->lock)
#define GST_WS_SINK_LOCK(obj)       (g_mutex_lock(GST_WS_SINK_GET_LOCK(obj)))
#define GST_WS_SINK_TRYLOCK(obj)    (g_mutex_trylock(GST_WS_SINK_GET_LOCK(obj)))
#define GST_WS_SINK_UNLOCK(obj)     (g_mutex_unlock(GST_WS_SINK_GET_LOCK(obj)))

#define GST_WS_SINK_BIT_GET_LOCK(obj)   (&GST_WS_SINK_CAST(obj)->bit_lock)
#define GST_WS_SINK_BIT_LOCK(obj)       (g_mutex_lock(GST_WS_SINK_BIT_GET_LOCK(obj)))
#define GST_WS_SINK_BIT_TRYLOCK(obj)    (g_mutex_trylock(GST_WS_SINK_BIT_GET_LOCK(obj)))
#define GST_WS_SINK_BIT_UNLOCK(obj)     (g_mutex_unlock(GST_WS_SINK_BIT_GET_LOCK(obj)))

#define GST_WS_SINK_TXT_GET_LOCK(obj)   (&GST_WS_SINK_CAST(obj)->txt_lock)
#define GST_WS_SINK_TXT_LOCK(obj)       (g_mutex_lock(GST_WS_SINK_TXT_GET_LOCK(obj)))
#define GST_WS_SINK_TXT_TRYLOCK(obj)    (g_mutex_trylock(GST_WS_SINK_TXT_GET_LOCK(obj)))
#define GST_WS_SINK_TXT_UNLOCK(obj)     (g_mutex_unlock(GST_WS_SINK_TXT_GET_LOCK(obj)))



#define GST_WS_SINK_GET_COND(obj)   (&GST_WS_SINK_CAST(obj)->cond)
#define GST_WS_SINK_WAIT(obj)       \
      g_cond_wait (GST_WS_SINK_GET_PREROLL_COND (obj), GST_WS_SINK_GET_COND (obj))
#define GST_WS_SINK_WAIT_UNTIL(obj, end_time) \
      g_cond_wait_until (GST_WS_SINK_GET_COND (obj), GST_WS_SINK_GET_COND (obj), end_time)
#define GST_WS_SINK_SIGNAL(obj)     g_cond_signal (GST_WS_SINK_GET_COND (obj));
#define GST_WS_SINK_BROADCAST(obj)  g_cond_broadcast (GST_WS_SINK_GET_COND (obj));

typedef struct _GstWsSink GstWsSink;
typedef struct _GstWsSinkClass GstWsSinkClass;
typedef struct _GstWsSinkPrivate GstWsSinkPrivate;

/**
 * GstWsSink:
 *
 * The opaque #GstWsSink data structure.
 */
struct _GstWsSink {
  GstElement     element;

  /*< protected >*/
  GstPad        *sinkpad_bit;
  GstPad        *sinkpad_txt;

  /*< private >*/ /* with LOCK */
  gboolean       running;
  GMutex         lock;
  GCond          cond;

  // bufferlist for sink pad data
  GstBufferList *bit_list;
  GMutex bit_lock;
  GstBufferList *txt_list;
  GMutex txt_lock;

  // task for websock
  GstTask *task;
  GRecMutex task_lock;

  // wsclient
  WsClientHandle wsclient_handle;
  gchar* wss_uri;
  int wsc_id;

  /*< private >*/
  GstWsSinkPrivate *priv;

  gpointer _gst_reserved[GST_PADDING_LARGE];
};

/**
 * GstWsSinkClass:
 * @parent_class: Element parent class
 */
struct _GstWsSinkClass {
  GstElementClass parent_class;

  /*< private >*/
  gpointer       _gst_reserved[GST_PADDING_LARGE];
};

GType           gst_ws_sink_get_type (void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstWsSink, gst_object_unref)
#endif

G_END_DECLS

#endif /* __GST_WS_SINK_H__ */
