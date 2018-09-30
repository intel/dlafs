#!/usr/bin/env node
"use strict"; // http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const https = require('https');
var spawn = require('child_process').spawn;

var client_id = 0;
var loop_times = 10000;
var pipe_num = 0;


const path_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
});

const path_wss = new WebSocketServer({server: path_server, path: '/sendPath'});
path_wss.on('connection', function(ws) {

    console.log('/sendPath connected');
    ws.on('message', function(path) {

        console.log("receive message:" + path);
        if(path.indexOf('stream=')==0){
            gst_cmd_path=path.substring(7);
            console.log('path: ' + gst_cmd_path);
            
            //client_id++;
            //gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path + ' ' + loop_times;
            //gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path;

	    //console.log('gst_cmd = ' + gst_cmd);
            //console.log('please write loop times on client');
            ws.send('stream source is done: ' + gst_cmd_path);
        } else if(path.indexOf('loop=')==0){
            loop_times = parseInt(path.substring(5));
	    //gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path + ' ' + loop_times;
            console.log('loop_times = ' + loop_times);
        } else if(path.indexOf('pipenum=')==0) {
            pipe_num = parseInt(path.substring(8))
            console.log('pipe_num = ' + pipe_num);
            //TODO: support multiple pipe
            pipe_num=1;
        }

        if((loop_times>0) && (pipe_num>0)) {
            for(var i=0; i<pipe_num; i++) {
                gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path + ' ' + loop_times;
                client_id++;

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
            ws.send('setup pipe done!');
            loop_times = 0;
            pipe_num = 0;
       }
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
let gst_cmd_path = 0;

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
    console.log("new pipeline joined!",client_id);
    console.log("this is "+ userArray.indexOf(ws)+"th loop time");
  }

  ws.on('message', function incoming(data) {
    // Broadcast to everyone else.
     console.log(data);
    /*wssBinaryEchoWithSize.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }*/
      if (receive_client.readyState === WebSocket.OPEN){
        var head = client_id+','+userArray.indexOf(ws);
        receive_client.send(head);
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

var exec = require('child_process').exec;
exec('hostname -I', function(error, stdout, stderr) {
    console.log('Please make sure to copy the ip address: ' + stdout);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log('exec error: ' + error);
    }
});
