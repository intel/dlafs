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

#ifndef SOCKET_CLIENT_APPPROTOCOL_H
#define SOCKET_CLIENT_APPPROTOCOL_H

#include <string>
#include <vector>
#include <memory>
#include <list>

using namespace std;
typedef struct ipcProtocol
{
    uint32_t iType;
    string  sPayload;
    int size(){
        return 4 + sPayload.size();
    };
} ipcProtocol;

class AppProtocol {
public:
    //format the struct to protocol buffer
    static void format(ipcProtocol &tMsg, string& sBuf);
    //parse the protocol buffer to struct
    static int parse(const char* pBuff, size_t iLength, list<ipcProtocol>& lMsgs);
private:
};


#endif //SOCKET_CLIENT_APPPROTOCOL_H
