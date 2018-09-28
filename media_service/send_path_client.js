#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

var fs = require('fs')
  , filename = 'path.txt';
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
const ws = new WebSocket("wss://"+url+":8126/sendPath", {
    rejectUnauthorized: false
  });


function input_stream_source() {
    rl.question('Please input the stream source: ', (answer) => {
        console.log(answer);
        if(answer.trim()!==""){
            console.log(`stream source is: ${answer}`);
            ws.send(answer);
        } else {
            var buf='~/1600x1200.mp4';
            console.log(`use default stream source: ${buf}`);
            ws.send(buf);
        }
    });
}

ws.on('open', function () {
    console.log(`[SEND_PATH_CLIENT] open()`);
    input_stream_source();
//  console.log(`[SEND_PATH_CLIENT] path send!`);
});

ws.on('message',function(data){
	console.log(`${data} `);

	if(ws.readyState === WebSocket.OPEN){

        rl.question('How many times do you want pipe run? ', (answer) => {
           console.log(parseInt(answer));
        	if(!isNaN(parseInt(answer))){
                    console.log(`We will let pipe run ${answer} times`);
                    ws.send(answer);
        	} else {
                    console.log('Pleae type right format');
                    rl.close();
                    ws.close();
        	}

        });
    }

});


ws.on('error', function () {
    console.log(`connect wrong!`);
}); 

ws.on('close', function () {
    console.log(`Right now I'll clear all pipes!`);
});
}


