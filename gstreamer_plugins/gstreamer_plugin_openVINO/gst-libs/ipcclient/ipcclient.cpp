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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "Looper.h"
#include "iostream"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "AppProtocol.h"
#include <algorithm>

 #include <glib.h>
#include <gst/gst.h>
#include "../ws/wsclient.h"
#include <string.h>
#include <sstream>

using namespace std;
#include <thread>
#include <string>

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
};

 struct _ipcclient{
      shared_ptr<Transceiver> pTrans;
      Looper* pLooper;
     struct sockaddr_un server;
     int sockfd;
     int id;
     GAsyncQueue *message_queue;
 };
 typedef struct _ipcclient IPCClient;
 
static void item_free_func(gpointer data)
{
       MessageItem *item = ( MessageItem *)data;
      if(item && (item->len>0)){
            g_free(item->data);
       }
       return;
}

 WsClientHandle wsclient_setup(const char *serverUri, int client_id)
 {
     IPCClient *ipcclient = (IPCClient *)g_new0 (IPCClient, 1);
     if(!ipcclient) {
         g_print("Failed to calloc IPCClient!\n");
         return 0;
     }

    GST_INFO("%s() - serverUri = %s, client_id = %d\n",__func__, serverUri, client_id);

    if(!serverUri) {
        g_print("Error: invalid uri of socket server!\n");
        return 0;
    }
     ipcclient->sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
     if (ipcclient->sockfd < 0) {
        g_print("Error:  failed to open socket - uri = %s\n", serverUri);
        return 0;
    }

     ipcclient->server.sun_family = AF_UNIX;
     g_stpcpy (ipcclient->server.sun_path, serverUri);
     
     if (connect( ipcclient->sockfd, (struct sockaddr *) & ipcclient->server, sizeof(struct sockaddr_un)) < 0) {
         close( ipcclient->sockfd);
         g_print("Error: failed to connect with stream socket\n");
         return 0;
     }

     ipcclient->id = client_id;
     ipcclient->message_queue = g_async_queue_new_full (item_free_func);

     GST_INFO("Connect to %s, pipe_id = %d\n",  serverUri , client_id);
     ipcclient->pTrans = make_shared<Transceiver>(ipcclient->sockfd);
     ipcclient->pLooper = new Looper(ipcclient->sockfd, ipcclient->pTrans, ipcclient->message_queue);
     ipcclient->pLooper->start();
     std::string sPipeID;
     ipcProtocol tInitMsg;
     tInitMsg.iType = ePipeID;
     //tInitMsg.sPayload = "{ \"type\": 1 \"payload\": " + string(argv[2]) + "}";
     //tInitMsg.sPayload = std::string("pipe_id=") + std::to_string(client_id);
     //char  id_info[4];
     //*((int *)id_info)= ipcclient->id;
     tInitMsg.sPayload = std::to_string( ipcclient->id);
     std::string sBuff = "";
     AppProtocol::format(tInitMsg, sBuff);
     GST_INFO(" Send 1st data: size = %ld, type = %d, sPayload = %s\n",  sBuff.size(), tInitMsg.iType,  tInitMsg.sPayload.c_str());
     ipcclient->pTrans->writeToSendBuffer(sBuff);
     ipcclient->pLooper->notify(ipcclient->pTrans);

     return (WsClientHandle)ipcclient;
 }
 
 void wsclient_send_data(WsClientHandle handle, char *data, int len)
 {
     IPCClient *ipcclient = (IPCClient *)handle;
 
     if(!handle) {
         g_print("Invalid IPCClientHandle!!!\n");
         return;
     }
     std::string sBuff = "";
     std::string message;
     ipcProtocol tMsg;
     if(len>1024)
        tMsg.iType = eMetaJPG;
     else
         tMsg.iType = eMetaText;
     //fill tMsg.sPayload with data
     tMsg.sPayload =  std::string(data, len);
     AppProtocol::format(tMsg, sBuff);
     ipcclient->pTrans->writeToSendBuffer(sBuff);
     ipcclient->pLooper->notify(ipcclient->pTrans);
 }

int wsclient_send_infer_data(WsClientHandle handle, InferenceData *infer_data, guint64 pts)
{
    std::ostringstream   data_str; 
    float ts = pts/1000000.0;
    data_str  << "ts="   << ts << "s,";
    data_str  << "prob="  << infer_data->probility << ",";
    data_str  << "name=" << infer_data->label << ",";
    data_str  << "rect=("  << infer_data->rect.x << "," << infer_data->rect.y << ")@" << infer_data->rect.width << "x" << infer_data->rect.height;
    data_str  << std::endl;
    std::string str = data_str.str();

    const char*txt_cache = str.c_str();
    int data_len = str.size();
    wsclient_send_data(handle, (char *)txt_cache, data_len);
    g_print("send data size=%d, %s\n",data_len, txt_cache);

   return data_len;
}

void wsclient_set_id(WsClientHandle handle,  int id)
{
    IPCClient *ipcclient = (IPCClient *)handle;

    if(!handle) {
        g_print("%s(): Invalid IPCClientHandle!!!\n",__func__);
        return;
    }
    ipcclient->id = id;
    return;
}

int wsclient_get_id(WsClientHandle handle)
{
    IPCClient *ipcclient = (IPCClient *)handle;

    if(!handle) {
        g_print("%s(): Invalid IPCClientHandle!!!\n",__func__);
        return -1;
    }
    return ipcclient->id;
}

/**
  * Pops data from the queue.
  * This function blocks until data become available.
  **/
MessageItem *wsclient_get_data(WsClientHandle handle)
{
    IPCClient *ipcclient = (IPCClient *)handle;
    if(!handle) {
        g_print("%s(): Invalid IPCClientHandle!!!\n",__func__);
        return NULL;
    }
    MessageItem *item =(MessageItem *)g_async_queue_pop(ipcclient->message_queue);

    return item;
}

 /**
   * Pops data from the queue.
   * If no data is received before end_time, NULL is returned.
   **/
 MessageItem *wsclient_get_data_timed(WsClientHandle handle)
 {
     IPCClient *ipcclient = (IPCClient *)handle;
     MessageItem *item = NULL;
     gint64 timeout_microsec = 400000; //400ms
     if(!handle) {
        g_print("%s(): Invalid IPCClientHandle!!!\n",__func__);
        return NULL;
    }
     item = (MessageItem *)g_async_queue_timeout_pop(ipcclient->message_queue, timeout_microsec);
     return item;
 }

void wsclient_free_item(MessageItem *item)
{
        item_free_func(item);
}
 void wsclient_destroy(WsClientHandle handle)
 {
     IPCClient *ipcclient = (IPCClient *)handle;
     if(!handle)
         return;

      if(ipcclient->pLooper) {
        ipcclient->pLooper->quit();
        delete ipcclient->pLooper;
        ipcclient->pLooper=NULL;
     }

    if(ipcclient->message_queue)
         g_async_queue_unref (ipcclient->message_queue);
 
     g_free(handle);
 }
 
#ifdef __cplusplus
 };
#endif

