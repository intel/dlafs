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
#include <interface/videodefs.h>

#include <glib.h>
#include <gst/gst.h>
#include "ipcclient.h"
#include <string.h>
#include <sstream>

#include "jsonpacker.h"

using namespace std;
#include <thread>
#include <string>

#ifdef __cplusplus
 extern "C" {
#endif

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

 IPCClientHandle ipcclient_setup(const char *serverUri, int client_id)
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
     ipcclient->pLooper = new Looper(ipcclient->pTrans, ipcclient->message_queue);
     ipcclient->pLooper->start();
     std::string sPipeID;
     ipcProtocol tInitMsg;
     tInitMsg.iType = ePipeID;

     tInitMsg.sPayload = std::to_string( ipcclient->id);
     GST_INFO(" Send 1st data: size = %d, type = %d, sPayload = %s\n",
        tInitMsg.size(), tInitMsg.iType,  tInitMsg.sPayload.c_str());
     ipcclient->pLooper->push(tInitMsg);

     return (IPCClientHandle)ipcclient;
 }
 
 void ipcclient_send_data(IPCClientHandle handle, const char *data, int len, enum ePlayloadType type)
 {
     IPCClient *ipcclient = (IPCClient *)handle;
 
     if(!handle) {
         g_print("Invalid IPCClientHandle!!!\n");
         return;
     }
     std::string sBuff = "";
     std::string message;
     ipcProtocol tMsg;
     tMsg.iType = type;
     //fill Payload with data
     tMsg.sPayload = std::string(data, len);
     ipcclient->pLooper->push(tMsg);
 }


 void ipcclient_upload_error_info(IPCClientHandle handle, const char *error_info)
 {
     std::string info = std::string(error_info);
     ipcclient_send_data(handle, error_info, info.size(), eErrorInfo);
 }

#if 0
static std::string covert_infer_data_to_string(void *data, guint64 pts, int infer_index)
{
    InferenceData *infer_data = (InferenceData *)data;
    std::ostringstream   data_str; 
    double ts = pts/1000000000.0;
    data_str  << "frame_index=" << infer_data->frame_index <<", ";
    data_str  << "infer_index=" << infer_index <<", ";
    data_str  << "ts="   << ts << "s,";
    data_str  << "prob="  << infer_data->probility << ",";
    data_str  << "label=" << infer_data->label << ",";
    data_str  << "rect=("  << infer_data->rect.x << ","
              << infer_data->rect.y << ")@" << infer_data->rect.width
              << "x" << infer_data->rect.height;
    data_str  << std::endl;
    std::string str = data_str.str();

    return str;
}
#endif
static std::string covert_infer_data_to_json_string(void *data, guint64 pts, int infer_index)
{
    InferenceData *infer_data = (InferenceData *)data;
    JsonPackage json_data;

    json_data.add_object_int("frame_index",infer_data->frame_index);
    json_data.add_object_int("infer_index",infer_index);
    json_data.add_object_int64("pts",pts);
    json_data.add_object_double("prob",infer_data->probility);
    json_data.add_object_string("label",infer_data->label);
    json_data.add_object_rect("rect", infer_data->rect.x, infer_data->rect.y,
                                     infer_data->rect.width, infer_data->rect.height);
    const char* string_data = json_data.object_to_string();

    if(string_data)
        return std::string(string_data) + std::string("\n");
    else
        return std::string("none data!\n");
}


/* meta data json format
  *   {
  *       frame_index:<int>
  *       index_index:<int>
  *       pts:<int64>
  *       obj_count:<int>
  *       objects [
  *            {
  *                 prob:<double>
  *                 label:<string>
  *                 rect {
  *                     x:<int>
  *                     y:<int>
  *                     w:<int>
  *                     h:<int>
  *                 }
  *            }
    *          {
  *                 prob:<double>
  *                 label:<string>
  *                 rect {
  *                     x:<int>
  *                     y:<int>
  *                     w:<int>
  *                     h:<int>
  *                 }
  *            }
  *            ...
  *      ]
  *
  */
static std::string covert_infer_data_to_json_string_full_frame(void *data, int count, guint64 pts, int infer_index)
{
    InferenceData *infer_data = (InferenceData *)data;
    JsonPackage json_data;
    struct json_object* array_obj = NULL;
    struct json_object* obj = NULL, *sub_obj = NULL, *rect_obj = NULL;

    json_data.add_object_int("frame_index",infer_data->frame_index);
    json_data.add_object_int("infer_index",infer_index);
    json_data.add_object_int64("pts",pts);
    json_data.add_object_int("obj_count",count);

    array_obj = json_object_new_array();
    for(int i=0; i<count; i++) {
        obj = json_object_new_object();
        sub_obj = json_object_new_double(infer_data->probility);
        json_object_object_add(obj, "prob", sub_obj);

        sub_obj = json_object_new_string(infer_data->label);
        json_object_object_add(obj, "label", sub_obj);

        rect_obj = json_object_new_object();
        sub_obj = json_object_new_int(infer_data->rect.x);
        json_object_object_add(rect_obj, "x", sub_obj);
        sub_obj = json_object_new_int(infer_data->rect.y);
        json_object_object_add(rect_obj, "y", sub_obj);
        sub_obj = json_object_new_int(infer_data->rect.width);
        json_object_object_add(rect_obj, "w", sub_obj);
        sub_obj = json_object_new_int(infer_data->rect.height);
        json_object_object_add(rect_obj, "h", sub_obj);
        json_object_object_add(obj, "rect", rect_obj);

        json_object_array_add(array_obj, obj);
        infer_data++;
    }
    json_data.add_object_object("objects",array_obj);
    const char* string_data = json_data.object_to_string();

    if(string_data)
        return std::string(string_data) + std::string("\n");
    else
        return std::string("none data!\n");
}


int ipcclient_send_infer_data(IPCClientHandle handle, void *data, guint64 pts, int infer_index)
{
#if 0
    std::string str = covert_infer_data_to_string(data, pts, infer_index);
#else
    std::string str = covert_infer_data_to_json_string(data, pts, infer_index);
#endif
    const char*txt_cache = str.c_str();
    int data_len = str.size();
    ipcclient_send_data(handle, (const char *)txt_cache, data_len, eMetaText);
    g_print("send data size=%d, %s\n",data_len, txt_cache);

    //debug
    //ipcclient_upload_error_info(handle, (char *)txt_cache);

   return data_len;
}


//int ipcclient_send_infer_data_full_frame(IPCClientHandle handle, void *data, int count, guint64 pts, int infer_index)
int ipcclient_send_infer_data_full_frame(void* file_handler, void *data, int count, guint64 pts, int infer_index)
{
	FILE* fH = (FILE*)file_handler;
    std::string str = covert_infer_data_to_json_string_full_frame(data, count, pts, infer_index);
    const char* txt_cache = str.c_str();
    int data_len = str.size();
	fputs(txt_cache, fH);
    //ipcclient_send_data(handle, (const char *)txt_cache, data_len, eMetaText);
    //g_print("send data size=%d, %s\n",data_len, txt_cache);

    //debug
    //ipcclient_upload_error_info(handle, (char *)txt_cache);

   return data_len;
}


void ipcclient_set_id(IPCClientHandle handle,  int id)
{
    IPCClient *ipcclient = (IPCClient *)handle;

    if(!handle) {
        g_print("%s(): Invalid IPCClientHandle!!!\n",__func__);
        return;
    }
    ipcclient->id = id;
    return;
}

int ipcclient_get_id(IPCClientHandle handle)
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
MessageItem *ipcclient_get_data(IPCClientHandle handle)
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
 MessageItem *ipcclient_get_data_timed(IPCClientHandle handle)
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

void ipcclient_free_item(MessageItem *item)
{
        item_free_func(item);
}
 void ipcclient_destroy(IPCClientHandle handle)
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

