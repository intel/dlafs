/*
 *Copyright (C) 2018-2019 Intel Corporation
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

#include <iostream>
#include <sys/time.h>
#include <cmath>
#include <arpa/inet.h>
#include "Looper.h"
#include "../ws/wsclient.h"
#include <string.h>

Looper::Looper(shared_ptr<Transceiver> pTrans,  GAsyncQueue *receive_queue) : _bQuit(false)
{
    _tEpoller.create(1);

    int iLooperFD = pTrans->getFD();

    receive_message_queue = receive_queue;

    if (iLooperFD == -1) {
        g_print("Please provide valid FD\n");
    }

    _tEpoller.add(iLooperFD, iLooperFD, EPOLLIN | EPOLLOUT);
    this->_pTrans = pTrans;
}

Looper::~Looper() {
    if (!_bQuit) {
        quit();
    }
    if (_tLooperThread.joinable()) {
         _tLooperThread.join();
    }
}

void Looper::start() {
    _tLooperThread = std::thread(&Looper::run, this);
}
void Looper::run() {
    while (!_bQuit) {
        try {
            int iEventNum = _tEpoller.wait(1000);

            for (int n = 0; n < iEventNum; ++n) {
                const epoll_event &ev = _tEpoller.get(n);
                if (ev.events & (EPOLLERR | EPOLLHUP))
                {
                    _pTrans->close();
                } else if (ev.events > 0)
                {
                    if (ev.events & EPOLLIN) {
                        handleRead();
                    }
                    if(ev.events & EPOLLOUT){
                        handleSend();
                    }
                }
            }
        }
        catch (std::exception &ex) {
            std::cout << "Event Error:" << ex.what() << std::endl;
        }
        catch (...) {}
    }
    // send the all buffers left, but not drop before exit
    if(_bQuit)
        flush();
}

void Looper::quit() {
    _bQuit = true;
}

void Looper::flush()
{
    if(_pTrans->isValid())
    {
        while (pop2Buffer() && _pTrans->handleResponse());
    }
}
void Looper::handleSend()
{
    if(_pTrans->isValid())
    {
        while (_pTrans->handleResponse() >= 0 && pop2Buffer());
    }
}

void Looper::handleRead()
{
    if(_pTrans->isValid())
    {
        list<ipcProtocol> lMsg;
        _pTrans->handleRequest(lMsg);
        for(auto tMsg : lMsg) {
            GST_INFO("\x1b[35mParse Msg from Server: %s \x1b[0m\n", tMsg.sPayload.substr(0,10).c_str());
            MessageItem *item = g_new0(MessageItem, 1);
            item->len = tMsg.sPayload.size();
            item->data = g_new0(char,  item->len);
            g_memmove(item->data,  tMsg.sPayload.c_str(),  item->len);
            g_async_queue_push ( receive_message_queue, item);
        }
    }
}

void Looper::notify()
{
    if (_pTrans->isValid())
    {
        _tEpoller.modify(_pTrans->getFD(), _pTrans->getFD(), EPOLLOUT | EPOLLIN);
    }
}

void Looper::push(ipcProtocol& tMsg)
{
    if (_pTrans->isValid())
    {
        _qSndQueue.push(tMsg);
        notify();
    }
}

bool Looper::pop2Buffer()
{
    ipcProtocol tMsg;
    bool bRet = false;
    if(_qSndQueue.tryPop(tMsg))
        bRet = _pTrans->sendMsg(tMsg);
    return bRet;
}
