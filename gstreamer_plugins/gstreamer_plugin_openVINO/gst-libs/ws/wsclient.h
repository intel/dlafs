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

#ifndef __GST_WS_CLIENT_H__
#define __GST_WS_CLIENT_H__

#include <interface/videodefs.h>
 
#ifdef __cplusplus
 extern "C" {
#endif
 
 typedef void * WsClientHandle;
 typedef struct MessageItem_{
     char *data;
     gint len;
 }MessageItem;
 
 WsClientHandle wsclient_setup(const char *serverUri, int client_id);
 void wsclient_send_data(WsClientHandle handle, char *data, int len);
 int wsclient_send_infer_data(WsClientHandle handle, InferenceData *infer_data, guint64 pts);
 void wsclient_destroy(WsClientHandle handle);
 MessageItem * wsclient_get_data(WsClientHandle handle);
 MessageItem *wsclient_get_data_timed(WsClientHandle handle);
 void wsclient_free_item(MessageItem *item);
 void wsclient_set_id(WsClientHandle handle,  int id);
 int wsclient_get_id(WsClientHandle handle);
#ifdef __cplusplus
 };
#endif
 
#endif
 
