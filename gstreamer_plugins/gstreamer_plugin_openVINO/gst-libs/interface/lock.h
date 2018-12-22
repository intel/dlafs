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

#ifndef _LOCK_H_
#define _LOCK_H_

#include <pthread.h>

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(className) \
    className(const className&);            \
    className& operator=(const className&);
#endif

namespace HDDLStreamFilter {

class Lock
{
public:
    Lock()
    {
        pthread_mutex_init(&m_lock, NULL);
    }

    ~Lock()
    {
        pthread_mutex_destroy(&m_lock);
    }

    void lock()
    {
        pthread_mutex_lock(&m_lock);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_lock);
    }
    pthread_mutex_t* get()
    {
        return &m_lock;
    }
    friend class Condition;

private:
    pthread_mutex_t m_lock;
    DISALLOW_COPY_AND_ASSIGN(Lock);
};

class AutoLock
{
public:
    explicit AutoLock(Lock& lock) : m_lock(lock)
    {
        m_lock.lock();
    }
    ~AutoLock()
    {
        m_lock.unlock();
    }
    Lock& m_lock;
private:
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};
}
#endif
