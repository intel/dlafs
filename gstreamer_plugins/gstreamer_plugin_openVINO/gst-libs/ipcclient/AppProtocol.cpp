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
#include "AppProtocol.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
using namespace std;

int AppProtocol::parse(const char* pBuff, size_t iLength, list<ipcProtocol>& lMsgs)
{
    size_t pos = 0;
    while(pos < iLength)
    {
        uint32_t iLen = iLength - pos;
        if(iLen < 4)
        {
            // not enough buffer
            break;
        }
        uint32_t iHeaderLen;
        iHeaderLen = ntohl(*(uint32_t*)(pBuff + pos));
        cout << "\niHeaderLen" << iHeaderLen << endl;
        if(iLen < iHeaderLen)
        {
            cout << "not enough payload, \nRemaining Buffer to parse " << iLen << "\ncurrent pos" << pos  << "\n current buffer size" << iLength << endl;
            break;
        }
        else
        {
            ipcProtocol tMsg;
            // read msg type
            tMsg.iType = ntohl(*(uint32_t*)(pBuff + pos+4));
            // read msg payload
            tMsg.sPayload = string(pBuff + pos+8, iHeaderLen-8);
            pos += iHeaderLen;
            lMsgs.push_back(tMsg);
        }

    }

    return pos;


}

void AppProtocol::format(ipcProtocol &tMsg, string& sBuf)
{
    uint32_t iHeaderLen = htonl(4+tMsg.size());
    uint32_t iType = htonl(tMsg.iType);
    sBuf.clear();
    sBuf.reserve(4 + tMsg.size());
    sBuf.append((const char*)&iHeaderLen, 4);
    sBuf.append((const char*)&iType, 4);
    sBuf.append(tMsg.sPayload);

}