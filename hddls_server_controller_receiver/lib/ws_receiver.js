//Copyright (C) 2018 Intel Corporation
// 
//SPDX-License-Identifier: MIT
//
//MIT License
//
//Copyright (c) 2018 Intel Corporation
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation the rights to
//use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
//AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
'use strict';
const crypto = require('crypto');
const fileHelper = require('./file_helper');
exports.websocketParser = function websocketParser(message) {
    var result;
    if(message instanceof Buffer && message.length != 0) {
        result = bufParser(message);
    } else {
        result = fileHelper.safelyJSONParser(message);
    }
    return result;
}

function bufParser(buf) {
    let headerDelimiter = buf.indexOf(1);
    let headers = fileHelper.safelyJSONParser(buf.slice(0, headerDelimiter).toString());
    if(headers == null)
        return null
    let delimiterChecksum = buf.lastIndexOf(1);
    var result =  {
        headers : headers,
        payload: buf.slice(headerDelimiter+1, delimiterChecksum),
        checkSum: buf.slice(delimiterChecksum + 1, buf.length).toString()
    }
    var hash = crypto.createHash('md5').setEncoding('hex');
    hash.update(result.payload);
    let localCheckSum = hash.digest('hex');
    result.checkSumPass = (localCheckSum == result.checkSum);

    return result;
}

function sendMessage(ws, message, code) {
    ws.send({headers: {method: 'text', code: code || 200}, payload: `${message}`});
}
