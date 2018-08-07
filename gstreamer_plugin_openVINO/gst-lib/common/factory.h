/*
 * Copyright (c) 2017 Intel Corporation
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

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <map>
#include <string>
#include <vector>

template < class T >
class Factory {
public:
    typedef T* Type;
    typedef Type (*Creator) (void);
    typedef std::string KeyType;
    typedef std::vector<KeyType> Keys;
    typedef std::map<KeyType, Creator> Creators;
    typedef typename Creators::iterator iterator;
    typedef typename Creators::const_iterator const_iterator;

    /**
     * Register class C with @param key.  Returns true if class C is
     * successfully registered using @param key.  If @param key is already
     * registered, returns false and does not register class C with
     * @param key.
     */
    template < class C >
    static bool register_(const KeyType& key)
    {
        std::pair<iterator, bool> result =
            getCreators().insert(std::make_pair(key, create<C>));
        return result.second;
    }

    /**
     * Create and return a new object that is registered with @param key.
     * If @param key does not exist, then returns NULL.  The caller is
     * responsible for managing the lifetime of the returned object.
     */
    static Type create(const KeyType& key)
    {
        Creators& creators = getCreators();
        const const_iterator creator(creators.find(key));
        if (creator != creators.end())
            return creator->second();
        return NULL;
    }

    static Keys keys()
    {
        Keys result;
        const Creators& creators = getCreators();
        const const_iterator endIt(creators.end());
        for (const_iterator it(creators.begin()); it != endIt; ++it)
            result.push_back(it->first);
        return result;
    }

private:
    template < class C >
    static Type create()
    {
        return new C;
    }
    static Creators& getCreators()
    {
        static Creators creators;
        return creators;
    }
};

#endif // __FACTORY_H__
