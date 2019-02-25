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

#ifndef __GST_IPC_SINK_H__
#define __GST_IPC_SINK_H__

#include <gst/gst.h>
#include <ipcclient/ipcclient.h>

G_BEGIN_DECLS


#define GST_TYPE_IPC_SINK              (gst_ipc_sink_get_type())
#define GST_IPC_SINK(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_IPC_SINK,GstIpcSink))
#define GST_IPC_SINK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_IPC_SINK,GstIpcSinkClass))
#define GST_IPC_SINK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_IPC_SINK, GstIpcSinkClass))
#define GST_IS_IPC_SINK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_IPC_SINK))
#define GST_IS_IPC_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_IPC_SINK))
#define GST_IPC_SINK_CAST(obj)         ((GstIpcSink *) (obj))

/**
 * GST_IPC_SINK_PAD:
 * @obj: ipc sink instance
 *
 * Gives the pointer to the #GstPad object of the element.
 */
#define GST_IPC_SINK_PAD(obj)          (GST_IPC_SINK_CAST (obj)->sinkpad)

#define GST_IPC_SINK_GET_LOCK(obj)   (&GST_IPC_SINK_CAST(obj)->lock)
#define GST_IPC_SINK_LOCK(obj)       (g_mutex_lock(GST_IPC_SINK_GET_LOCK(obj)))
#define GST_IPC_SINK_TRYLOCK(obj)    (g_mutex_trylock(GST_IPC_SINK_GET_LOCK(obj)))
#define GST_IPC_SINK_UNLOCK(obj)     (g_mutex_unlock(GST_IPC_SINK_GET_LOCK(obj)))

#define GST_IPC_SINK_BIT_GET_LOCK(obj)   (&GST_IPC_SINK_CAST(obj)->bit_lock)
#define GST_IPC_SINK_BIT_LOCK(obj)       (g_mutex_lock(GST_IPC_SINK_BIT_GET_LOCK(obj)))
#define GST_IPC_SINK_BIT_TRYLOCK(obj)    (g_mutex_trylock(GST_IPC_SINK_BIT_GET_LOCK(obj)))
#define GST_IPC_SINK_BIT_UNLOCK(obj)     (g_mutex_unlock(GST_IPC_SINK_BIT_GET_LOCK(obj)))

#define GST_IPC_SINK_TXT_GET_LOCK(obj)   (&GST_IPC_SINK_CAST(obj)->txt_lock)
#define GST_IPC_SINK_TXT_LOCK(obj)       (g_mutex_lock(GST_IPC_SINK_TXT_GET_LOCK(obj)))
#define GST_IPC_SINK_TXT_TRYLOCK(obj)    (g_mutex_trylock(GST_IPC_SINK_TXT_GET_LOCK(obj)))
#define GST_IPC_SINK_TXT_UNLOCK(obj)     (g_mutex_unlock(GST_IPC_SINK_TXT_GET_LOCK(obj)))



#define GST_IPC_SINK_GET_COND(obj)   (&GST_IPC_SINK_CAST(obj)->cond)
#define GST_IPC_SINK_WAIT(obj)       \
      g_cond_wait (GST_IPC_SINK_GET_PREROLL_COND (obj), GST_IPC_SINK_GET_COND (obj))
#define GST_IPC_SINK_WAIT_UNTIL(obj, end_time) \
      g_cond_wait_until (GST_IPC_SINK_GET_COND (obj), GST_IPC_SINK_GET_COND (obj), end_time)
#define GST_IPC_SINK_SIGNAL(obj)     g_cond_signal (GST_IPC_SINK_GET_COND (obj));
#define GST_IPC_SINK_BROADCAST(obj)  g_cond_broadcast (GST_IPC_SINK_GET_COND (obj));

typedef struct _GstIpcSink GstIpcSink;
typedef struct _GstIpcSinkClass GstIpcSinkClass;
typedef struct _GstIpcSinkPrivate GstIpcSinkPrivate;

/**
 * GstIpcSink:
 *
 * The opaque #GstIpcSink data structure.
 */
struct _GstIpcSink {
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

  // ipc client
  IPCClientHandle ipc_handle;
  IPCClientHandle ipc_handle_proxy;
  gchar* ipcs_uri;
  int ipcc_id;

  /*< private >*/
  GstIpcSinkPrivate *priv;
  gint meta_data_index;
  gint bit_data_index;
  gint frame_index;

  gpointer _gst_reserved[GST_PADDING_LARGE];
};

/**
 * GstIpcSinkClass:
 * @parent_class: Element parent class
 */
struct _GstIpcSinkClass {
  GstElementClass parent_class;

  /*< private >*/
  gpointer       _gst_reserved[GST_PADDING_LARGE];
};

GType           gst_ipc_sink_get_type (void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstIpcSink, gst_object_unref)
#endif

G_END_DECLS

#endif /* __GST_IPC_SINK_H__ */
