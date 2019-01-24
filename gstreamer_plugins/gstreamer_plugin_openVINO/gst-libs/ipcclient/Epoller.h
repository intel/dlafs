
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

#ifndef __EPOLLER_H
#define __EPOLLER_H

#include <sys/epoll.h>
#include <cassert>
#include <memory>
#include <unistd.h>

class Epoller {
  Epoller(const Epoller& other) = delete;
  Epoller& operator = (const Epoller&) = delete;
 public:

  Epoller(bool bET = true);

  ~ Epoller();

  void create(int iMaxConn);

  void add(int iFD, long long lData, unsigned int iEvent);

  void modify(int iFD, long long lData, unsigned int iEvent);

  void dele(int iFD, long long lData, unsigned int iEvent);

  //millisecond
  int wait(int iMs);

  struct epoll_event& get(int iIndex) {assert(_pPrevs !=0); return _pPrevs[iIndex];}

 protected:

  void ctrl(int iFD, long long lData, unsigned int iEvents, int iOP);

 protected:


  int _iEpollFD;

  int _iMaxConn;

  epoll_event* _pPrevs;

  bool _bET;

};


#endif //TIMER_EPOLLER_H
