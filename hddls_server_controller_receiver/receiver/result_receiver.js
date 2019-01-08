#!/usr/bin/env nodejs
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
const fs = require('fs');
const fileHelper = require('../lib/file_helper');
const constants = require('../lib/constants');
const SecureWebsocket = require('../lib/secure_client');

var options = {
  host: '127.0.0.1',
  port: 8445,
  ca: fs.readFileSync('./cert_client_8126_8124/ca-crt.pem'),
  checkServerIdentity: () => undefined,
  key: fs.readFileSync('./cert_client_8126_8124/client1-key.pem'),
  cert: fs.readFileSync('./cert_client_8126_8124/client1-crt.pem'),
  route: 'data'
};

var pipe2file = new Map();
for (let i = 0; i < 100; i++) {
  pipe2file.set(i, 0);
}

ws = new SecureWebsocket(options);

function incoming(data) {
  var metaData = Buffer.from(data.payload);
  console.log("payload type %s", typeof data.payload);
  console.log("receive type %s payload %s", data.headers.type, Buffer.isBuffer(metaData));
  var pipe_id = data.headers.pipe_id;
  var con = 'pipe_' + pipe_id.toString();
  let path = './' + con;
  if (data.headers.type == constants.msgType.eMetaJPG) {
      console.log('save JPG');
      let temp = pipe2file.get(pipe_id);
      let image_name = 'image_' + temp + '.jpg';
      let path = './' + con + '/' + image_name;
      fileHelper.saveBuffer(path, metaData);
      temp++;
      pipe2file.set(pipe_id, temp);
  } else if(data.headers.type == constants.msgType.eMetaText) {
      console.log('save txt');
      let path = './' + con + '/output.txt';
      fs.appendFile(path, metaData, function (err) {
          if (err) {
              console.log("append failed: ", err);
          } else {
              console.log("done".green);
          }
      })
    }
}

ws.on('message', incoming);
