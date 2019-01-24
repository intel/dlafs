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


#ifndef SOCKET_CLIENT_TRANSCEIVER_H
#define SOCKET_CLIENT_TRANSCEIVER_H

#include <cstdint>
#include <string>
#include <mutex>
#include "AppProtocol.h"
using namespace std;

class Transceiver {
  Transceiver(const Transceiver& other) = delete;
  Transceiver& operator = (const Transceiver&) = delete;
 public:
  Transceiver(int iFD);
  ~Transceiver();
  void reInit(int iFD);
  int send(const void* buf, uint32_t len, uint32_t flag);
  int recv(void* buf, uint32_t len, uint32_t flag);
  void close();
  bool isValid();
  int handleRequest(list<ipcProtocol>& lMsgs);
  int handleResponse();
  int getFD(){return _iFD;};
  void writeToSendBuffer(const string& msg);
  bool sendMsg(ipcProtocol& tMsg);
 private:
  int _iFD;
  string _sSendBuf;
  string _sRecvBuf;

};

#endif //SOCKET_CLIENT_TRANSCEIVER_H
