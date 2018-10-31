/*
 * Copyright (c) 2018 Intel Corporation
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

#ifndef __GST_WS_CLIENT_H__
#define __GST_WS_CLIENT_H__
 
#ifdef __cplusplus
 extern "C" {
#endif
 
 typedef void * WsClientHandle;
 typedef struct MessageItem_{
     char *data;
     gint len;
 }MessageItem;
 
 WsClientHandle wsclient_setup(char *serverUri, int client_id);
 void wsclient_send_data(WsClientHandle handle, char *data, int len);
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
 
