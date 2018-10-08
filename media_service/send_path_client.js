#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');


var program = require('commander');
program
    .option('-s, --stream <string>', 'video stream source address')
    .option('-l, --loop <n>', 'loop times')
    .option('-n, --num <n>', 'pipe number')
    .parse(process.argv);
var loop_times = 0;
var pipe_num = 0;
var stream_path = "";


program.parse(process.argv)
if(program.stream) {
    stream_path = program.stream;
    console.log('stream_path = ' + stream_path);
}
if(program.loop) {
    loop_times = program.loop;
    console.log('loop_times = ' + loop_times);
}
if(program.num) {
    pipe_num = program.num;
    console.log('pipe_num = ' + pipe_num);
}

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
    if(stream_path=="") {
        rl.question('Please input the stream source: ', (answer) => {
            console.log(answer);
            if(answer.trim()!==""){
                console.log('stream source is: ' + answer);
                ws.send('stream=' + answer);
            } else {
                console.log('invalid stream source: '+ answer + ', please reenter again!');
                return 0;
            }
        });
    } else {
        console.log('stream source is: '+ stream_path);
        ws.send('stream='+stream_path);
    }
    return 1;
}

var set_loop_times_enable = 1;
function input_loop_times() {
    if(set_loop_times_enable==0)
        return;
    if(loop_times==0) {
        rl.question('How many times do you want pipe run? ', (answer) => {
           console.log(parseInt(answer));
                if(!isNaN(parseInt(answer))){
                    console.log(`We will let pipe run ${answer} times`);
                    ws.send('loop='+answer);
                } else {
                    console.log('Pleae type right format');
                    rl.close();
                    ws.close();
                }

        });
    } else {
        console.log(`We will let pipe run ${loop_times} times`);
        ws.send('loop='+loop_times);
        set_loop_times_enable = 0;
    }
}


var set_pipe_number_enable = 1;
function input_pipe_number() {
    if(set_pipe_number_enable==0)
        return;

    if(pipe_num==0) {
        rl.question('How many pipes do you want run? ', (answer) => {
           console.log(parseInt(answer));
                if(!isNaN(parseInt(answer))){
                    console.log(`We will let ${answer} pipes to run`);
                    ws.send('pipenum='+answer);
                } else {
                    console.log('Please type right format');
                    rl.close();
                    ws.close();
                }

        });
    } else {
        console.log(`We will let ${pipe_num} pipes to run`);
        ws.send('pipenum='+pipe_num);
        set_pipe_number_enable = 0;
    }
}

ws.on('open', function () {
    console.log(`[SEND_PATH_CLIENT] open()`);
    var ret = input_stream_source();
    //console.log("input_stream_source return: " + ret);
});

ws.on('message',function(data){
	console.log(`message = ${data} `);
    if(ws.readyState == WebSocket.OPEN){
        if(data.indexOf('stream source is done')>=0) {
            input_loop_times();
        } else if(data.indexOf('set loop times done')>=0) {
            input_pipe_number();
        } else if(data.indexOf('setup pipe done')>=0) {
            rl.question('Do you want to quit(y/n)? ', (answer) => {
                if(answer.trim()=='y') {
                    console.log('quit client!');
                    rl.close();
                    ws.close();
                }
            });
        }
    }
});


ws.on('error', function () {
    console.log(`connect wrong!`);
}); 

ws.on('close', function () {
    console.log(`Right now I'll clear all pipes!`);
});
}


