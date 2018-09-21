#!/usr/bin/env node
const WebSocket = require('ws');
var ab2str = require('arraybuffer-to-string');
const fs = require('fs');
var con='';

let count=0;
const ws = new WebSocket("wss://localhost:8123/binaryEchoWithSize?id=1", {
    rejectUnauthorized: false
});

ws.on('open', function () {
    console.log(`[RECEIVE_DATA_CLIENT] open()`);
});

ws.on('message', function (data) {
    console.log("[RECEIVE_DATA_CLIENT] Received:",data);
    if(typeof data ==='string'){
      console.log('looks like we got a new pipe, hello pipe_',data);
      if(data.indexOf('=')>0)
         return;
      con='pipe_'+data;
      var path = './'+con;
      console.log("output dir: ", path);
      fs.access(path,function(err){
          //console.log("access error = ", err);
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

     } else {

         if(data.byteLength >1024){
         count++;
         var buff =  new Buffer(data);
         var image_name='image_'+count+'.jpg';
         var path = './'+ con + '/' + image_name;
         var fd = fs.openSync(path, 'w');
         fs.write(fd, buff, 0, buff.length, 0, function(err, written) {
              console.log('err', err);
              console.log('written', written);
         });

    } else {

         var uint8 = new Uint8Array(data);
         var path = './'+ con + '/output.txt';
         fs.appendFile(path, ab2str(uint8)+ "\n", function (err) {
               if (err) {
                   console.log("append failed");
               } else {
                    // done
                    console.log("done");
               }
          })
      }
   }

});

ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 
 

