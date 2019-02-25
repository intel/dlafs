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


#ifndef __GST_IPC_CLIENT_H__
#define __GST_IPC_CLIENT_H__

#ifdef __cplusplus
 extern "C" {
#endif

 enum ePlayloadType{
     ePipeID = 0,
     ePipeCreate = 1,
     ePipeProperty = 2,
     ePipeDestroy = 3,
     eMetaJPG = 4,
     eMetaText = 5,
     eErrorInfo = 6,
 };

 typedef void * IPCClientHandle;
 typedef struct MessageItem_{
     char *data;
     gint len;
 }MessageItem;
 
 IPCClientHandle ipcclient_setup(const char *serverUri, int client_id);
 void ipcclient_send_data(IPCClientHandle handle, const char *data, int len, enum ePlayloadType type);
 int ipcclient_send_infer_data(IPCClientHandle handle, void *infer_data, guint64 pts, int infer_index);
 void ipcclient_destroy(IPCClientHandle handle);
 MessageItem * ipcclient_get_data(IPCClientHandle handle);
 MessageItem *ipcclient_get_data_timed(IPCClientHandle handle);
 void ipcclient_free_item(MessageItem *item);
 void ipcclient_set_id(IPCClientHandle handle,  int id);
 int ipcclient_get_id(IPCClientHandle handle);
 void ipcclient_upload_error_info(IPCClientHandle handle, const char *error_info);
#ifdef __cplusplus
 };
#endif
 
#endif
 
