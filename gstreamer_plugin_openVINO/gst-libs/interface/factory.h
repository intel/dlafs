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

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <map>
#include <string>
#include <vector>

template < class T >
class Factory {
public:
    typedef T*  (*AllocatorFunc) (void);
    typedef std::map<std::string, AllocatorFunc> AllocatorMap;
    typedef typename AllocatorMap::iterator iterator;
    typedef typename AllocatorMap::const_iterator const_iterator;

    template < class C >
    static bool register_(const  std::string&  key)
    {
        AllocatorMap &allocatorMap = getAllocatorMap();
        std::pair<iterator, bool> result =
           allocatorMap.insert(std::make_pair(key, allocate<C>));
        return result.second;
    }

    static T* create(const std::string& key)
    {
        AllocatorMap &allocatorMap = getAllocatorMap();
        const const_iterator creator(allocatorMap.find(key));
        if (creator != allocatorMap.end())
            return creator->second();
        return NULL;
    }

    static std::vector<std::string> get_keys()
    {
        std::vector<std::string> result;
        AllocatorMap &allocatorMap = getAllocatorMap();
        const const_iterator endIt(allocatorMap.end());
        for (const_iterator it(allocatorMap.begin()); it != endIt; ++it)
            result.push_back(it->first);
        return result;
    }

private:
    template < class C >
    static T* allocate()
    {
        return new C;
    }
     static AllocatorMap& getAllocatorMap()
    {
         static AllocatorMap allocatorMap;
         return allocatorMap;
    }
};

#endif // __FACTORY_H__
