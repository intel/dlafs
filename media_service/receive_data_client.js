#!/usr/bin/env node
const WebSocket = require('ws');
var ab2str = require('arraybuffer-to-string');
const fs = require('fs');
const path = require('path');
var rimraf = require('rimraf');

var con='';
let count=0;
var client_id = 0;

var m = new Map();
for(let i=0;i<100;i++){
  m.set(i,0);
}


function mkdirs(dirpath) {
    if (!fs.existsSync(path.dirname(dirpath))) {
      mkdirs(path.dirname(dirpath));
    }
    fs.mkdirSync(dirpath);
}


var filename = 'path.txt';
var url = 0;
fs.readFile(filename, 'utf8', function(err, data) {
  if (err) throw err;
  console.log('found: ' + filename);
  console.log('server ip is:'+ data);
  url = data;
  url= url.replace(/[\r\n]/g,"");  
});

setTimeout(() => {
    set_websocket();
  }, 2000);

function set_websocket(){
const ws = new WebSocket("wss://"+url+":8123/binaryEchoWithSize?id=1", {
    rejectUnauthorized: false
});


ws.on('open', function () {
    console.log(`[RECEIVE_DATA_CLIENT] open()`);
});

ws.on('message', function (data) {
    console.log("[RECEIVE_DATA_CLIENT] Received:",data);
    count = 0;
    if(typeof data ==='string'){
      if(data.indexOf('=')>0)
         return;
    }

    client_id=data[0];
    con='pipe_'+client_id.toString();
    var path = './'+con;
    console.log("output dir: ", path);
    fs.access(path,function(err){
        console.log("access error = ", err);
        if(err){
           console.log("please build a new directory for ",con);
           fs.mkdir(con, function (err) {
              if(err) {
                  console.log("Failed to create dir, err =  ", err);
              }
          });
        } else {
            console.log("output data will be put into ", path);
            return;
        }
    });

    if(data.byteLength >1024){
         var temp = m.get(client_id);
         var buff =  new Buffer(data);
         var image_name='image_'+temp+'.jpg';
         var path = './'+ con + '/' + image_name;
         var fd = fs.openSync(path, 'w');
         fs.write(fd, buff, 4, buff.length-4, 0, function(err, written) {
             console.log('err', err);
             console.log('written', written);
         });
         temp++;
         m.set(client_id,temp);
     } else {
         var buff =  new Buffer(data);
         var path = './'+ con + '/output.txt';
         fs.appendFile(path, buff.toString('utf8',4,buff.length)+ "\n", function (err) {
             if (err) {
                 console.log("append failed");
             } else {
                 // done
                 console.log("done");
             }
         })
    }
});

ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 
}

