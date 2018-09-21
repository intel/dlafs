#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

const ws = new WebSocket("wss://localhost:8126/sendPath", {
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
	console.log(`right now we have ${data} pipes`);

	if(ws.readyState === WebSocket.OPEN){

        rl.question('How many pipes do you want to start? ', (answer) => {
           console.log(parseInt(answer));
        	if(!isNaN(parseInt(answer))){
                    console.log(`We will start new ${answer} pipes`);
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

