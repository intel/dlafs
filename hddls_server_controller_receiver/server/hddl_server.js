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
const SecureServer = require('../lib/secure_wss_server');
const fs = require('fs');
var privateKey  = fs.readFileSync('./cert_server_8126_8124/server-key.pem', 'utf8');
var certificate = fs.readFileSync('./cert_server_8126_8124/server-crt.pem', 'utf8');
var ca = fs.readFileSync('./cert_server_8126_8124/ca-crt.pem', 'utf8');
var crl = null;
if(fs.existsSync('./cert_server_8126_8124/client1.crl')) {
  crl = fs.readFileSync('./cert_server_8126_8124/client1.crl')
}
const constants = require("../lib/constants");
const fileHelper = require("../lib/file_helper")
var options = {
  port: 8445,
  key: privateKey,
  cert: certificate,
  requestCert: true,
  ca: [ca],
  metaPath: __dirname + '/models/model_info.json',
  ipcProtocol: "raw"
};

if(crl != null) {
  options.crl = crl;
}

if (process.platform === "win32") {
  options.socket = "\\\\.\\pipe\\" + "ipc";
} else {
  options.socket = '/tmp/unix.sock';
}
var pipe2file = new Map();
for (let i = 0; i < 100; i++) {
  pipe2file.set(i, 0);
}
const route = require('../lib/router');
var server = new SecureServer(options);


var routes = {
  "text": (ws,message, adminCtx)=>{console.log(`Message from client ${message.payload}`);ws.send(JSON.stringify(message));},
  "create": route.createHandler,
  "destroy": route.destroyHandler,
  "property": route.propertyHandler,
  "model": route.updateModel
}
var router = route.router(routes);
function incoming(ws, message, adminCtx) {
  let fn = router(message);
  !!fn && fn(ws, message, adminCtx);
};

function unixApp(data, adminCtx){
    if(data.type == constants.msgType.ePipeID && adminCtx.pipe2pid.has(parseInt(data.payload))) {
      console.log("valid pipe %s", data.payload);
      var createJSON;
      createJSON = adminCtx.pipe2json.get(parseInt(data.payload)).create;
      var initConfig = JSON.stringify(createJSON);
      adminCtx.transceiver.send(initConfig);
    } else {
      !!adminCtx.dataCons && adminCtx.dataCons.readyState === adminCtx.dataCons.OPEN && adminCtx.dataCons.send(JSON.stringify({headers: {type: data.type, pipe_id: adminCtx.transceiver.id}, payload: data.payload}));
      //!!adminCtx.dataCons && fileHelper.uploadBuffer(data.payload, adminCtx.dataCons, {headers: {type: data.type, pipe_id: transceiver.id}});
    }
}

//Temp solution for websocket-based ipc, later will transfer to more unix socket ipc
server.unixUse(unixApp);
server.adminUse(incoming);

server.start();

process.on('SIGINT', ()=> {
  console.log('Server close due to exit');
  server.close();
});