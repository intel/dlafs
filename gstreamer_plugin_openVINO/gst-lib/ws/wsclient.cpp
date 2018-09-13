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


#include <gst/gst.h>
#include "wsclient.h"
#include <uWS/uWS.h>

using namespace std;
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif

struct _wsclient{
    uWS::WebSocket<uWS::CLIENT> *client;
    //uWS::Hub hub;
};
typedef struct _wsclient WsClient;

#define DEFAULT_WSS_URI "wss://localhost:8123/binaryEchoWithSize"

WsClientHandle wsclient_setup(char *serverUri)
{
    WsClient *wsclient = (WsClient *)g_new0 (WsClient, 1);
    if(!wsclient) {
        g_print("Failed to calloc WsClient!\n");
        return 0;
    }
    wsclient->client = NULL;

    std::thread t([&wsclient, serverUri]{
        uWS::Hub hub;

        hub.onConnection([wsclient, serverUri](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
            wsclient->client = ws;
            g_print("SUCCESS to CONNECTE with %s!\n", serverUri);
        });

        hub.onError([](void *user) {
            g_print("FAILURE: Connection failed! Timeout?\n");
            exit(-1);
        });

        hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
            g_print("CLIENT CLOSE: %d: %s\n", code, message);
        });

        if(!serverUri){
            hub.connect(DEFAULT_WSS_URI, nullptr);
            g_print("Try to default websocket server: %s\n", DEFAULT_WSS_URI);
            /*
                    g_print("\n\nError: no websocker server!\n");
                    g_print("Please set WebSocket Server URI:\n");
                    g_print("\tExample: wssuri=wss://localhost:8123/binaryEchoWithSize\n");
                    exit(-1);
                  */
        }else{
            hub.connect(serverUri, nullptr);
            g_print("Try to connect websocket server: %s\n", serverUri);
        }
        hub.run();
    });
    t.detach();

    while(!wsclient->client)
        g_usleep(1000);

    return (WsClientHandle)wsclient;
}

void wsclient_send_data(WsClientHandle handle, char *data, int len)
{
    WsClient *wsclient = (WsClient *)handle;
    //uWS::WebSocket<uWS::CLIENT> *client;

    if(!handle) {
        g_print("Invalid WsClientHandle!!!\n");
        return;
    }
    wsclient->client->send(data, len, uWS::OpCode::BINARY);
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

    g_free(handle);
}

#ifdef __cplusplus
};
#endif
