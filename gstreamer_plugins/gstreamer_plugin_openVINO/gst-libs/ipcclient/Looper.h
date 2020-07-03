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

#ifndef SOCKET_CLIENT_LOOPER_H
#define SOCKET_CLIENT_LOOPER_H

#include <thread>
#include <condition_variable>
#include <functional>
#include <vector>
#include <map>
#include <string>
#include <memory>

#include <glib.h>
#include <gst/gst.h>

#include "Transceiver.h"
#include "Epoller.h"
#include "ThreadSafeQueue.h"
class Looper
{
  Looper(const Looper& other) = delete;
  Looper& operator = (const Looper&) = delete;
 public:
  Looper(shared_ptr<Transceiver> pTrans, GAsyncQueue *receive_queue);
  ~Looper();
  void run();
  void start();
  void quit();
  void notify();
  void push(ipcProtocol& tMsg);
  void flush();
  //unsigned long int == uint64_t

 private:
  std::thread _tLooperThread;
  ThreadSafeQueue<ipcProtocol> _qSndQueue;
  volatile bool _bQuit;
  Epoller _tEpoller;
  GAsyncQueue *receive_message_queue;
  gint64 _iSendDuration;

 private:
  void handleRead();
  void handleSend();
  bool pop2Buffer();
  shared_ptr<Transceiver> _pTrans;
};


#endif //SOCKET_CLIENT_LOOPER_H
