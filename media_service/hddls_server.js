#!/usr/bin/env node
"use strict"; 
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const https = require('https');
const spawn = require('child_process').spawn;
const kill = require('tree-kill');


let client_id = 0;
let loop_times = 10000;
let pipe_num = 0;
let child_pid = 0;
let pipeid_to_number = 0;
let pipe_all = "";
let contin_loop_times = 0;


let pipe_map = new Map();
for(let i=0;i<100;i++){
  pipe_map.set(i,0);
}


const path_server = https.createServer({
    key: fs.readFileSync('./cert_server_8126/server-key.pem'),
    cert: fs.readFileSync('./cert_server_8126/server-crt.pem'),
    ca: fs.readFileSync('./cert_server_8126/ca-crt.pem'),
    requestCert: true,
    rejectUnauthorized: true
});

const path_wss = new WebSocketServer({server: path_server, path: '/sendPath',verifyClient: ClientVerify});
path_wss.on('connection', function(ws) {

    console.log('controller connected !' .bgYellow);
    client_id++;
    ws.send('client id is '+ client_id);   
    client_pipe = "";

    ws.on('message', function(path) {

        console.log("receive message:" + path);
        console.log(typeof path);
        if(path.indexOf('stream=')==0){
            gst_cmd_path=path.substring(7);
            console.log('path: ' + gst_cmd_path);
            //gst_cmd = 'hddlspipe ' + client_id + ' ' + gst_cmd_path + ' ' + loop_times;
            ws.send('stream source is done: ' + gst_cmd_path);
        } else if(path.indexOf('loop=')==0){
            loop_times = parseInt(path.substring(5));
            console.log('loop_times = ' + loop_times);
            ws.send('set loop times done!');
        } else if(path.indexOf('pipenum=')==0) {
            pipe_num = parseInt(path.substring(8))
            console.log('pipe_num = ' + pipe_num);
        } else if (path.indexOf(',')>-1){

          console.log(path);
          let arr = path.split(',');
          if(arr[1]==='p'){
            pipeid_to_number = parseInt(arr[0]);
            child_pid = pipe_map.get(pipeid_to_number);
            kill(child_pid);
            console.log('we killed pipe'+ pipeid_to_number);

          }else if (arr[1]==='c'){
               console.log(pipe_num);

               pipeid_to_number = parseInt(arr[0]);
               contin_loop_times = parseInt(arr[2]);
               //gst_cmd = 'hddlspipe ' + pipeid_to_number + ' ' + gst_cmd_path + ' ' + contin_loop_times;
               gst_cmd = 'hddlspipes -i  ' + pipeid_to_number + ' -l ' + contin_loop_times;
                let child = spawn(gst_cmd , {
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

                client_pipe = client_pipe + pipe_count.toString() + ",";
                pipe_id = pipe_count;
                console.log(pipe_id);
                console.log(pipe_count);
                pipe_count++;

                let child = spawn(gst_cmd , {
                    shell: true
                });

          client_map.set(client_id,client_pipe);
          console.log(client_map);

        } else if(path[0]==='p'){
          property_json = JSON.parse(path.substring(1));
          ws_index = pipe_map.get(property_json.command_set_property.pipe_id);
          ws_index.send(path.substring(1));
            
        } else if(path[0]==='d') {
          console.log(pipe_map);
          destory_json = JSON.parse(path.substring(1));
          ws_index = pipe_map.get(destory_json.command_destroy.pipe_id);
          console.log(ws_index);
          //ws_index.close();
          console.log(path.substring(1));
          ws_index.send(path.substring(1));
          console.log('we killed pipe '+ destory_json.command_destroy.pipe_id);
          pipe_map.set(destory_json.command_destroy.pipe_id,-1);
          console.log(pipe_map);
          per_client_pipe = client_map.get(destory_json.client_id);
          per_client_pipe = per_client_pipe.replace(destory_json.command_destroy.pipe_id.toString()+",","");
          client_map.set(destory_json.client_id,per_client_pipe);
          console.log(client_map);
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
let pipe_client = 0;

const data_server = https.createServer({
  cert: fs.readFileSync('./cert_server_8123/server-cert.pem'),
  key: fs.readFileSync('./cert_server_8123/server-key.pem'),
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
     console.log("Receiver connected!" .bgMagenta);

  } else if (params["id"] == "3"){
     
  } else {

    userArray.push(ws);
    //console.log(ws);
    //console.log("new pipeline joined!",pipe_id);
    console.log("this is "+ userArray.indexOf(ws)+"th loop time");
    //pipe_id++;
  }
  

  ws.on('message', function incoming(data) {
    // Broadcast to everyone else.
     //console.log(data);
    /*wssBinaryEchoWithSize.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }*/
      if (typeof data ==="string"){
        console.log(data);
        pipe_client = data.split("=");
        console.log(pipe_client);
        pipe_id = parseInt(pipe_client[1]);
        console.log(pipe_id);
        pipe_map.set(pipe_id,ws);

      }
      else {
        if (receive_client.readyState === WebSocket.OPEN){
//        var head = client_id+','+userArray.indexOf(ws);
//        receive_client.send(head);
        receive_client.send(data);
      }
    }
  });

  ws.on('close', function (close) {
      console.log("client closed");
      
  });

});


function ClientVerify(info) {

   let ret = false;//refuse
   params = url.parse(info.req.url, true).query;

   if (params["id"] == "1") {
     if(params["key"] == "b"){
       ret = true;//pass
     }
   }

   if (params["id"] == "2") {
     if(params["key"] == "c"){
       ret = true;//pass
     }
   }

   if (params["id"] == "3") {
     ret = true;//pass
   }

   return ret;
}

path_server.listen(8126);
data_server.listen(8123);
console.log('Listening on port 8126 and 8123...');

const exec = require('child_process').exec;
exec('hostname -I', function(error, stdout, stderr) {
    console.log('Please make sure to copy the ip address into path.txt: ' + stdout);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log('exec error: ' + error);
    }
});

exec('hostname', function(error, stdout, stderr) {
    console.log('Please make sure to copy the DNS name into hostname.txt: ' + stdout);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log('exec error: ' + error);
    }
});



