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

#include <iostream>
#include "Epoller.h"
#include <glib.h>
#include <gst/gst.h>

Epoller::Epoller(bool bET) : _iEpollFD(-1), _iMaxConn(1024),_pPrevs(NULL), _bET(bET)
{}

Epoller::~Epoller()
{

  if(_iEpollFD > 0)
  {
    close(_iEpollFD);
  }
}

void Epoller::ctrl(int iFD, long long lData, unsigned int iEvents, int iOP)
{
  epoll_event ev;
  ev.data.u64 = lData;
  if(_bET)
  {
    ev.events = iEvents | EPOLLET;
  }
  else
  {
    ev.events = iEvents;
  }

  epoll_ctl(_iEpollFD, iOP, iFD, &ev);

}

void Epoller::create(int iMaxConn) {
  _iMaxConn = iMaxConn;
  try {
    _iEpollFD = epoll_create(_iMaxConn + 1);
  }
  catch (...)
  {
    GST_ERROR( "EPOLL CREATE FAIL\n");
  }



  if(_pPrevs)
  {
    delete[] _pPrevs;
  }

  _pPrevs = new epoll_event[_iMaxConn + 1];
}

void Epoller::add(int iFD, long long lData, unsigned int iEvent)
{
  ctrl(iFD, lData, iEvent, EPOLL_CTL_ADD);
}

void Epoller::modify(int iFD, long long lData, unsigned int iEvent)
{
  ctrl(iFD, lData, iEvent, EPOLL_CTL_MOD);
}

void Epoller::dele(int iFD, long long lData, unsigned int iEvent)
{
  ctrl(iFD, lData, iEvent, EPOLL_CTL_MOD);
}

int Epoller::wait(int iMs)
{
  return epoll_wait(_iEpollFD, _pPrevs, _iMaxConn + 1, iMs);
}