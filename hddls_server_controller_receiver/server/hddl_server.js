#!/usr/bin/env node
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
var privateKey  = fs.readFileSync('./server_cert/server-key.pem', 'utf8');
var certificate = fs.readFileSync('./server_cert/server-crt.pem', 'utf8');
var ca = fs.readFileSync('./server_cert/ca-crt.pem', 'utf8');
var crl = null;
if(fs.existsSync('./server_cert/client1.crl')) {
  crl = fs.readFileSync('./server_cert/client1.crl')
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
  options.socket = 'ipc_socket/unix.sock';
}

if(!fs.existsSync('ipc_socket')) {
  fs.mkdirSync('ipc_socket', {mode: 0o700});
}


var pipe2file = new Map();
for (let i = 0; i < 100; i++) {
  pipe2file.set(i, 0);
}
const route = require('../lib/router');
var server = new SecureServer(options);


const routes = {
  "text": (ws,message, adminCtx)=>{console.log(`Message from client ${message.payload}`);ws.send(JSON.stringify(message));},
  "create": route.createHandler,
  "destroy": route.destroyHandler,
  "property": route.propertyHandler,
  "model": route.updateModel
};

var router = route.router(routes);
function incoming(ws, message, adminCtx) {
  let fn = router(message);
  !!fn && fn(ws, message, adminCtx);
}

function unixApp(data, adminCtx, transceiver){
    if(data.type == constants.msgType.ePipeID && adminCtx.pipe2pid.has(parseInt(data.payload))) {
      console.log("valid pipe %s", data.payload);
      var createJSON;
      createJSON = adminCtx.pipe2json.get(parseInt(data.payload)).create;
      var initConfig = JSON.stringify(createJSON);
      transceiver.send(initConfig);
    } else if(data.type == constants.msgType.eErrorInfo) {
      if(transceiver.hasOwnProperty('id')) {
        if(adminCtx.pipe2pid.has(transceiver['id'])) {
          const controllerID = adminCtx.pipe2pid.get(transceiver['id'])['cid'] || null;
          if(controllerID != null) {
            if(adminCtx.wsConns.has(controllerID)) {
              const controllerWS = adminCtx.wsConns.get(controllerID);
              if(controllerWS.readyState === controllerWS.OPEN)
                controllerWS.send(JSON.stringify(
                  {
                    headers: {method: 'pipe_message', pipe_id: transceiver.id},
                    payload: data.payload.toString()
                  }));
            }
          }
        }
      }
    } else {
      if(!!adminCtx.dataCons && adminCtx.dataCons.readyState === adminCtx.dataCons.OPEN && transceiver.hasOwnProperty('id'))
        adminCtx.dataCons.send(
          JSON.stringify(
          {
            headers: {type: data.type, pipe_id: transceiver.id},
            payload: data.payload
          }));
    }
}

//Temp solution for websocket-based ipc, later will transfer to more unix socket ipc
server.unixUse(unixApp);
server.adminUse(incoming);

server.on('error', (error)=>{console.log("Server ERROR: " + error.message); server.close();process.exit(0)});

try {
  server.start();
} catch(error){
  console.log(error.message);
  server.close();
  process.exit(0);
}

process.on('SIGINT', ()=> {
  console.log('Server close due to exit');
  server.close();
});
