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

#ifndef __ALGO_QUEUE_H__
#define __ALGO_QUEUE_H__

#pragma once

#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

template<class T>
class thread_queue
{
public:
    thread_queue(int sz = 0x7FFFFFFF):_size_limit(sz), _max_size(0), _closed(false), _flush(false){}

    //with Filter on element(get specific element)
    template<class FilterFunc>
    bool get(T &ret, FilterFunc filter)
    {
        std::unique_lock<std::mutex> lk(_m);

        typename std::deque<T>::iterator it;
        _cv.wait(lk, [this, filter, &it]
        {
            //mutex auto relocked
            //check from the First Input to meet FIFO requirement
            for(it = _q.begin(); it != _q.end(); ++it)
                if(filter(*it)) return true;

            //if closed & not-found, then we will never found it in the future
            if(_closed) return true;
            if(_flush) {
                    _flush = false;
                    return true;
            }

            return false;
        });

        if ((int)_q.size() > _max_size) {
            _max_size = _q.size();
        }

        //nothing will found in the future
        //  if (it == _q.end() && _closed) {
        if (it == _q.end() || _closed) {
            return false;
        }

        ret = *it;          //copy construct the return value
        _q.erase(it);   //remove from deque

        if (_q.size() < _size_limit) {
            _cv_notfull.notify_all();
        }

        return true;
    }

    bool get(T &ret)
    {
        return get(ret, [](const T &) {return true;});
    }


    void put(const T & obj)
    {
        std::unique_lock<std::mutex> lk(_m);

        if (!_closed) {
            _cv_notfull.wait(lk, [this] {
                return (_q.size() <_size_limit) || _closed;
            });

            if (!_closed)
                _q.push_back(obj);
        }

        _cv.notify_all();
    }


    void close(void)
    {
        std::unique_lock<std::mutex> lk(_m);
        _closed = true;
        _cv.notify_all();
    }

    // release the lock, make it can go
    void flush(void)
    {
        std::unique_lock<std::mutex> lk(_m);
        _flush = true;
        _cv.notify_all();
    }

    int size(void){
        std::unique_lock<std::mutex> lk(_m);
        return _q.size();
    }
    int max_size(void){
        return _max_size;
    }
private:
    size_t                          _size_limit;
    std::deque<T>                   _q;
    std::mutex                      _m;
    std::condition_variable        _cv;
    std::condition_variable        _cv_notfull;
    int                            _max_size;
    bool                           _closed;
    bool                           _flush;
};
#endif
