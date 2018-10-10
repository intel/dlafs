#!/usr/bin/env node
"use strict"; 
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const https = require('https');
var spawn = require('child_process').spawn;
var kill = require('tree-kill');


var client_id = 0;
var loop_times = 10000;
var pipe_num = 0;
var child_pid = 0;
var pipeid_to_number = 0;
var pipe_all = "";
var contin_loop_times = 0;


var pipe_map = new Map();
for(let i=0;i<100;i++){
  pipe_map.set(i,0);
}


const path_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
});

const path_wss = new WebSocketServer({server: path_server, path: '/sendPath'});
path_wss.on('connection', function(ws) {

    console.log('/sendPath connected');
    ws.on('message', function(path) {

        console.log("receive message:" + path);
        console.log(typeof path);
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
            ws.send('set loop times done!');

        } else if(path.indexOf('pipenum=')==0) {

            pipe_num = parseInt(path.substring(8))
            console.log('pipe_num = ' + pipe_num);

        } else if (path.indexOf(',')>-1){

          console.log(path);
          var arr = path.split(',');
          if(arr[1]==='p'){
            pipeid_to_number = parseInt(arr[0]);
            child_pid = pipe_map.get(pipeid_to_number);
            kill(child_pid);
            console.log('we killed pipe'+ pipeid_to_number);

          }else if (arr[1]==='c'){
               console.log(pipe_num);

               pipeid_to_number = parseInt(arr[0]);
               contin_loop_times = parseInt(arr[2]);
               gst_cmd = 'hddlspipe ' + pipeid_to_number + ' ' + gst_cmd_path + ' ' + contin_loop_times;
                var child = spawn(gst_cmd , {
                    shell: true
                });
                pipe_map.set(pipeid_to_number,child.pid);

                child.stderr.on('data', function (data) {
                    console.error("STDERR:", data.toString());
                });

                child.stdout.on('data', function (data) {
                    console.log("STDOUT:", data.toString());
                });

                child.on('exit', function (exitCode) {
                    console.log("Child exited with code: " + exitCode);
                });
               console.log('process continue');
          }
          
        } 

        if((loop_times>0) && (pipe_num>0)) {
            for(var i=0; i<pipe_num; i++) {
                gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path + ' ' + loop_times;

                var child = spawn(gst_cmd , {
                    shell: true
                });

                pipe_map.set(client_id,child.pid);
                

                child.stderr.on('data', function (data) {
                    console.error("STDERR:", data.toString());
                });

                child.stdout.on('data', function (data) {
                    console.log("STDOUT:", data.toString());
                });

                child.on('exit', function (exitCode) {
                    console.log("Child exited with code: " + exitCode);
                });
                client_id++;
            }
            for(let i=0;pipe_map.get(i)!=0;i++){
              pipe_all = pipe_all+i+',';
            }
            //ws.send('setup pipe done!');
            //ws.send(pipe_map);
            ws.send(pipe_all);

            loop_times = 0;
            pipe_num = 0;

       }
    });

    ws.on('close', function() {
        pipe_all = "";
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
     //console.log(data);
    /*wssBinaryEchoWithSize.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }*/
      if (receive_client.readyState === WebSocket.OPEN){
//        var head = client_id+','+userArray.indexOf(ws);
//        receive_client.send(head);
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

