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

/*
  * Author: Zhang, Yi3
  * Email:  yi3.zhang@intel.com
  * Date: 2018.12.16
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

Looper::Looper(int iFD, shared_ptr<Transceiver> pTrans,  GAsyncQueue *receive_queue) : _bQuit(false)
{
    _tEpoller.create(1);

    _iLooperFD = iFD;
    receive_message_queue = receive_queue;

    if (_iLooperFD == -1) {
        g_print("Please provide valid FD\n");
    }

    _tEpoller.add(_iLooperFD, _iLooperFD, EPOLLIN | EPOLLOUT);
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

void Looper::initialize(int fd, shared_ptr<Transceiver> pTrans) {
    _tEpoller.create(1);

    _iLooperFD = fd;

    if (_iLooperFD == -1) {
        g_print("Please provide valid FD\n" );
    }

    _tEpoller.add(_iLooperFD, _iLooperFD, EPOLLIN | EPOLLOUT);
    this->_pTrans = pTrans;

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
                if (ev.events & EPOLLIN) {
                    handleRead();
                }
                if(ev.events & EPOLLOUT){
                    handleSend();
                }
            }
        }
        catch (std::exception &ex) {
            std::cout << "Event Error:" << ex.what() << std::endl;
        }
        catch (...) {}
    }
    close(_iLooperFD);
}

void Looper::quit() {
    std::lock_guard<std::mutex> guard(_mLock);
    _bQuit = true;
    _mCond.notify_all();
    if (_tLooperThread.joinable()) {
   	    _tLooperThread.join();
    }
}

void Looper::handleSend()
{
    if(_pTrans->isValid())
    {
        while(_pTrans->handleResponse()>0);
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

void Looper::notify(shared_ptr<Transceiver> pTrans) {
    if(pTrans->isValid())
    {
        _tEpoller.modify(pTrans->getFD(), pTrans->getFD(), EPOLLOUT|EPOLLIN);
    }
}

void Looper::handleRead(std::string buff) {
    std::string output = buff.empty() ? "Empty" : buff;
    GST_LOG("Receive Message: %s\n", output.c_str());
}
