#!/usr/bin/env nodejs
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
  metaPath: __dirname + '/model/model_info.json',
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
