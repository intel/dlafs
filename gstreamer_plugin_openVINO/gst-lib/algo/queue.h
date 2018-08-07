/*
 * Copyright (c) 2017-2018 Intel Corporation
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

/*
 * barrier_block_queue.h
 *
 *  Created on: Oct 17, 2017
 *      Author: xiping
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
    thread_queue(int sz = 0x7FFFFFFF):_size_limit(sz), _max_size(0), _closed(false){}

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

				return false;
			});

		if ((int)_q.size() > _max_size) {
			_max_size = _q.size();
		}

		//nothing will found in the future
//		if (it == _q.end() && _closed) {
		if (it == _q.end() || _closed) {
			return false;
		}

		ret = *it; 		//copy construct the return value
		_q.erase(it);	//remove from deque

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
};
#endif
