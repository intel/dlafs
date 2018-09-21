#!/usr/bin/env node
"use strict"; // http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const https = require('https');
var spawn = require('child_process').spawn;


const path_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
});

const path_wss = new WebSocketServer({server: path_server, path: '/sendPath'});
path_wss.on('connection', function(ws) {

    console.log('/sendPath connected');
    ws.on('message', function(path) {

        if(path.length > 3){

        console.log('received path: ' + path);
        gst_cmd='gst-launch-1.0 filesrc location=' + path + ' ! qtdemux ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res res.src_pic ! mfxjpegenc ! wssink name=ws res.src_txt ! ws.';
	      console.log('gst_cmd = ' + gst_cmd);

        } else {

        pipe  = pipe + parseInt(path);

        for(var i=0; i<parseInt(path);i++){

            var child = spawn(gst_cmd , {
                shell: true
            });

            child.stderr.on('data', function (data) {
                console.error("STDERR:", data.toString());
            });

            child.stdout.on('data', function (data) {
                 console.log("STDOUT:", data.toString());
             });

            child.on('exit', function (exitCode) {
                console.log("Child exited with code: " + exitCode);
            });
        }

      }
      ws.send(pipe);
    });

    ws.on('close', function() {
        console.log('/sendPath Connection closed!');
    });
    ws.on('error', function(e) {
        console.log('/sendPath Connection error!');
    });
});

const util = require('util');
const url = require('url');
let params = 0;
let userArray = [];
let receive_client = 0;
let pipe = 0;
let gst_cmd = 0;

const data_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem'),
  strictSSL: false
});

const data_wss = new WebSocketServer({server: data_server, path: '/binaryEchoWithSize',verifyClient: ClientVerify});
// Broadcast to all.
/*data_wss.broadcast = function broadcast(data) {
  data_wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};*/

data_wss.on('connection', function connection(ws) {

  if (params["id"] == "1") {

     receive_client = ws;
     console.log("client connected!");

  } else {

    userArray.push(ws);
    console.log("new pipeline joined!",userArray.indexOf(ws));
  }

  ws.on('message', function incoming(data) {
    // Broadcast to everyone else.
     console.log(data);
    /*wssBinaryEchoWithSize.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }*/
      if (receive_client.readyState === WebSocket.OPEN){
        receive_client.send(userArray.indexOf(ws));
        receive_client.send(data);
      }

  });

  ws.on('close', function (close) {

        console.log("client closed");

  });

});


function ClientVerify(info) {

   var ret = true;//拒绝

   params = url.parse(info.req.url, true).query;

   return ret;

}

path_server.listen(8126);
data_server.listen(8123);
console.log('Listening on port 8126 and 8123...');

