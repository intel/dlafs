#!/usr/bin/env node
"use strict"; 
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const fs = require('fs');
const https = require('https');
const spawn = require('child_process').spawn;
const colors = require('colors');

let client_id = 0;
let pipe_id = 0;
let create_json = "";
let destory_json = "";
let property_json = "";
let pipe_count = 0;
let client_pipe = "";
let per_client_pipe = "";
let ws_index = 0;
let temp_json_path = "";

let pipe_map = new Map();
for(let i=0;i<100;i++){
  pipe_map.set(i,0);
}

let client_map = new Map();
for(let i=0;i<100;i++){
  client_map.set(i,"");
}


const path_server = https.createServer({
    key: fs.readFileSync('./cert_server_8126/server-key.pem'),
    cert: fs.readFileSync('./cert_server_8126/server-crt.pem'),
    ca: fs.readFileSync('./cert_server_8126/ca-crt.pem'),
    requestCert: true,
    rejectUnauthorized: true
});

const path_wss = new WebSocketServer({server: path_server, path: '/controller',verifyClient: ClientVerify});
path_wss.on('connection', function(ws) {

    console.log('controller connected !' .bgYellow);
    client_id++;
    ws.send(client_id);   
    //client_pipe = "";

    ws.on('message', function(path) {

        console.log("receive message:" + path);
        console.log(typeof path);
        if (path[0]==='c') {  
                   
           create_json = JSON.parse(path.substring(1));
           if(client_map.get(create_json.client_id)===""){
            client_pipe = "";
             
           }else{
             client_pipe = client_map.get(create_json.client_id);
           }

           for (let i = 0; i<create_json.command_create.pipe_num; i++){
            	gst_cmd = 'hddlspipes -i  ' + pipe_count + ' -l ' + create_json.command_create.loop_times;

                let child = spawn(gst_cmd , {
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

                client_pipe = client_pipe + pipe_count.toString() + ",";
                pipe_id = pipe_count;

                
                temp_json_path='./client_'+ create_json.client_id+'_temp_create.json';
                fs.writeFile(temp_json_path, JSON.stringify(create_json), {flag: 'w'}, function (err) { if(err) {
                    console.error("write file failed: ", err);
                    } else {
                       console.log('temp json: done');
                    }
                });
                console.log(pipe_id);
                console.log(pipe_count);
                pipe_count++;

          }

          client_map.set(create_json.client_id,client_pipe);
          //console.log(client_map);
          ws.send(client_map.get(create_json.client_id));

        } else if(path[0]==='p'){
          property_json = JSON.parse(path.substring(1));
          ws_index = pipe_map.get(property_json.command_set_property.pipe_id);
          ws_index.send(path.substring(1));
            
        } else if(path[0]==='d') {
          //console.log(pipe_map);
          destory_json = JSON.parse(path.substring(1));
          ws_index = pipe_map.get(destory_json.command_destroy.pipe_id);
          //console.log(ws_index);
          //ws_index.close();
          console.log(path.substring(1));
          ws_index.send(path.substring(1));         
          //console.log(pipe_map);
          per_client_pipe = client_map.get(destory_json.client_id);
          per_client_pipe = per_client_pipe.replace(destory_json.command_destroy.pipe_id.toString()+",","");
          client_map.set(destory_json.client_id,per_client_pipe);
          console.log(client_map);
          console.log('we killed pipe '+ destory_json.command_destroy.pipe_id);
          pipe_map.set(destory_json.command_destroy.pipe_id,-1);
          ws.send("we have killed pipe "+ destory_json.command_destroy.pipe_id);
          console.log("we have send to client" .bgRed);
          ws.send(client_map.get(destory_json.client_id));
          
        }

        
    });

    ws.on('close', function() {
        console.log('controller closed!' .bgYellow);
    });
    ws.on('error', function(e) {
        console.log('controller connection error!' .bgRed);
        console.log(e);
    });
});

//const util = require('util');
const url = require('url');
let params = 0;
let userArray = [];
let receive_client = 0;
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
     
     //temp_json_file='./temp_create.json';
     fs.readFile(temp_json_path, 'utf8', function(err, data) {
        if (err) throw err;
        console.log("read create.config: ", data);
        ws.send(data);
      });

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

        //temp_json_path='./client_'+ client_id+'_temp_create.json';
      }
      else {
        if (receive_client.readyState === WebSocket.OPEN){
//        var head = client_id+','+userArray.indexOf(ws);
//        receive_client.send(head);
        receive_client.send(data);
      }
    }
  });

  ws.on('error', function(e) {
    console.log('receiver connection error!' .bgRed);
    console.log(e);
});

  ws.on('close', function (ws) {
    if (params["id"] == "1") {
      console.log("Receiver closed!" .bgMagenta);
    } else {
      console.log("pipeline closed!" .bgMagenta);
    }
      
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
console.log('Listening on port 8126 and 8123...' .rainbow);

const exec = require('child_process').exec;
exec('hostname -I', function(error, stdout, stderr) {
    console.log(('Please make sure to copy the ip address into path.txt: ' + stdout).green);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log(('exec error: ' + error).red);
    }
});

exec('hostname', function(error, stdout, stderr) {
    console.log(('Please make sure to copy the DNS name into hostname.txt: ' + stdout).green);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log(('exec error: ' + error).red);
    }
});



