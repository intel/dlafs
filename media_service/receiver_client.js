#!/usr/bin/env node
const WebSocket = require('ws');
const ab2str = require('arraybuffer-to-string');
const fs = require('fs');
const path = require('path');
const rimraf = require('rimraf');
const readline = require('readline');

let con='';
let count=0;
let pipe_id = 0;
let psw = "";

let m = new Map();
for(let i=0;i<100;i++){
  m.set(i,0);
}

const rl = readline.createInterface({
 input: process.stdin,
 output: process.stdout
});


function mkdirs(dirpath) {
    if (!fs.existsSync(path.dirname(dirpath))) {
      mkdirs(path.dirname(dirpath));
    }
    fs.mkdirSync(dirpath);
}

const filename = 'path.txt';
let url = 0;
function read_server_ip(){
fs.readFile(filename, 'utf8', function(err, data) {
  if (err) throw err;
  console.log('found: ' + filename);
  console.log('server ip is:'+ data);
  url = data;
  url= url.replace(/[\r\n]/g,"");
  set_websocket();
});
}

function input_password() {

  rl.question('Please input password to connect server(default:b): ', (answer) => {
     if(answer !==""){
        psw = answer;
        read_server_ip();
     }
});

}


function set_websocket(){
  const ws = new WebSocket("wss://"+url+":8123/binaryEchoWithSize?id=1"+"&key="+psw, {
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

    
    pipe_id=data[0];
    con='pipe_'+pipe_id.toString();
    let path = './'+con;
    console.log("output dir: ", path);

    try{
       fs.accessSync(path,fs.constants.F_OK);
    }catch(ex){
        console.log("access error for ", con);
        if(ex){
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
    };

    if(data.byteLength >1024){
         let temp = m.get(pipe_id);
         let buff =  new Buffer(data);
         let image_name='image_'+temp+'.jpg';
         let path = './'+ con + '/' + image_name;
         let fd = fs.openSync(path, 'w');
         fs.write(fd, buff, 4, buff.length-4, 0, function(err, written) {
             console.log('err', err);
             console.log('written', written);
         });
         temp++;
         m.set(pipe_id,temp);
     } else {
         let buff =  new Buffer(data);
         let path = './'+ con + '/output.txt';
         fs.appendFile(path, buff.toString('utf8',4,buff.length)+ "\n", function (err) {
             if (err) {
                 console.log("append failed: ", err);
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

input_password();


