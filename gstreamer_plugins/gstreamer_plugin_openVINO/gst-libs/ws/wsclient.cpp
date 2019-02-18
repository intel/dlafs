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


#include <glib.h>
#include <gst/gst.h>
#include "wsclient.h"
#include <uWS/uWS.h>
#include <string.h>
#include <interface/videodefs.h>

using namespace std;
#include <thread>
#include <string>
#include <sstream>

#ifdef __cplusplus
 extern "C" {
#endif
 
 struct _wsclient{
     uWS::WebSocket<uWS::CLIENT> *client;
     int id;
     char *data;
     int data_len;
     GAsyncQueue *message_queue;
 };
 typedef struct _wsclient WsClient;
 
#define DEFAULT_WSS_URI "wss://localhost:8123/binaryEchoWithSize?id=3"

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
     WsClient *wsclient = (WsClient *)g_new0 (WsClient, 1);
     if(!wsclient) {
         GST_ERROR("Failed to calloc WsClient!\n");
         return 0;
     }
     wsclient->client = NULL;
     wsclient->id = client_id;
     wsclient->data = NULL;
     wsclient->data_len = 0;
     wsclient->message_queue = g_async_queue_new_full (item_free_func);

     std::thread t([&wsclient, serverUri, client_id]{
         uWS::Hub hub;
 
         hub.onConnection([wsclient, serverUri, client_id](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
             wsclient->client = ws;
             std::stringstream ss;
             std::string s;
             ss << client_id;
             ss >> s;
             std::string msg = std::string("pipe_id=") + s;
             ws->send(msg.c_str(), msg.size(), uWS::OpCode::TEXT);
             GST_INFO("Success to connect with %s!\n", serverUri);
         });

        // If receive message, put it to queue
        hub.onMessage([wsclient ](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode){
            GST_INFO("Receive receive message = %s!\n", message);
            if(opCode ==  uWS::OpCode::TEXT) {
                MessageItem *item = g_new0(MessageItem, 1);
                item->data = g_new0(char, length);
                item->len = length;
                g_memmove(item->data, message, length);
                g_async_queue_push ( wsclient->message_queue, item);
             }else{
                GST_INFO("Receive message that not need to be process: size = %ld\n", length);
             }
         });

         hub.onError([](void *user) {
             GST_ERROR("FAILURE: Connection failed! Timeout?\n");
             exit(-1);
         });
 
         hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
             GST_INFO("CLIENT CLOSE: %d: %s\n", code, message);
         });
 
         if(!serverUri){
             hub.connect(DEFAULT_WSS_URI, nullptr);
             GST_INFO("Try to default websocket server: %s\n", DEFAULT_WSS_URI);
         }else{
             hub.connect(serverUri, nullptr);
             GST_INFO("Try to connect websocket server: %s\n", serverUri);
         }
         hub.run();
     });
     t.detach();

    int times=1000;
     while(!wsclient->client  && times-->0)
         g_usleep(1000);

     return (WsClientHandle)wsclient;
 }
 
 void wsclient_send_data(WsClientHandle handle, const char *data, int len, enum ePlayloadType type)
 {
     WsClient *wsclient = (WsClient *)handle;
     if(!handle) {
         GST_ERROR("Invalid WsClientHandle!!!\n");
         return;
     }
     #if 0
     if(!wsclient->data) {
        wsclient->data_len = len + 4;
        wsclient->data = g_new0(char, wsclient->data_len);
     } else if(wsclient->data_len < len+4) {
        wsclient->data_len = len + 4;
        wsclient->data = g_try_renew(char, wsclient->data, wsclient->data_len);
     }
     *(int *)wsclient->data = wsclient->id;
     g_memmove(wsclient->data+4, data, len);
     wsclient->client->send(wsclient->data, len+4, uWS::OpCode::BINARY);
    #else
    char  id_info[4];
    int *data_info = (int *)id_info;
    *data_info= wsclient->id;
    std::string sBuff = std::string(id_info, 4) + std::string(data, len);
    //g_print("SendData: size = %ld, buf=%d\n", sBuff.size(),  *((int *)sBuff.c_str()));
    wsclient->client->send(sBuff.c_str(), len+4, uWS::OpCode::BINARY);
    #endif
 }


int wsclient_send_infer_data(WsClientHandle handle, void *data, guint64 pts,  int infer_index)
{
    InferenceData *infer_data = (InferenceData *)data;
    std::ostringstream   data_str; 
    float ts = pts/1000000000.0;
    data_str  << "frame=" << infer_data->frame_index <<", ";
    data_str  << "infer_index=" << infer_index <<", ";
    data_str  << "pts="   << ts << "s,";
    data_str  << "prob="  << infer_data->probility << ",";
    data_str  << "name=" << infer_data->label << ",";
    data_str  << "rect=("  << infer_data->rect.x << ","
              << infer_data->rect.y << ")@" << infer_data->rect.width
              << "x" << infer_data->rect.height;
    data_str  << std::endl;
    std::string str = data_str.str();

    const char*txt_cache = str.c_str();
    int data_len = str.size();
    wsclient_send_data(handle, (const char *)txt_cache, data_len, eMetaText);
    g_print("send data size=%d, %s\n",data_len, txt_cache);

   return data_len;
}

void wsclient_set_id(WsClientHandle handle,  int id)
{
    WsClient *wsclient = (WsClient *)handle;

    if(!handle) {
        GST_ERROR("%s(): Invalid WsClientHandle!!!\n",__func__);
        return;
    }
    wsclient->id = id;
    return;
}

int wsclient_get_id(WsClientHandle handle)
{
    WsClient *wsclient = (WsClient *)handle;

    if(!handle) {
        GST_ERROR("%s(): Invalid WsClientHandle!!!\n",__func__);
        return -1;
    }
    return wsclient->id;
}

/**
  * Pops data from the queue.
  * This function blocks until data become available.
  **/
MessageItem *wsclient_get_data(WsClientHandle handle)
{
    WsClient *wsclient = (WsClient *)handle;
    if(!handle) {
        GST_ERROR("%s(): Invalid WsClientHandle!!!\n",__func__);
        return NULL;
    }
    MessageItem *item =(MessageItem *)g_async_queue_pop(wsclient->message_queue);

    return item;
}

 /**
   * Pops data from the queue.
   * If no data is received before end_time, NULL is returned.
   **/
 MessageItem *wsclient_get_data_timed(WsClientHandle handle)
 {
     WsClient *wsclient = (WsClient *)handle;
     MessageItem *item = NULL;
     gint64 timeout_microsec = 400000; //400ms
     if(!handle) {
        GST_ERROR("%s(): Invalid WsClientHandle!!!\n",__func__);
        return NULL;
    }
     item = (MessageItem *)g_async_queue_timeout_pop(wsclient->message_queue, timeout_microsec);
     return item;
 }

void wsclient_free_item(MessageItem *item)
{
        item_free_func(item);
}
 void wsclient_destroy(WsClientHandle handle)
 {
     WsClient *wsclient = (WsClient *)handle;
 
     if(!handle)
         return;

     if(wsclient->client) {
        wsclient->client->close();
        g_usleep(1000);
     }

     if(wsclient->data)
         g_free(wsclient->data);

    if(wsclient->message_queue)
         g_async_queue_unref (wsclient->message_queue);
 
     g_free(handle);
 }
 
#ifdef __cplusplus
 };
#endif

