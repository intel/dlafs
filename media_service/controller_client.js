#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');


const program = require('commander');
program
    .option('-s, --stream <string>', 'video stream source address')
    .option('-l, --loop <n>', 'loop times')
    .option('-n, --num <n>', 'pipe number')
    .parse(process.argv);
let loop_times = 0;
let pipe_num = 0;
let stream_path = "";
let input_arr = "";
let rec_pipe_arr = "";
let is_input_valid = false;
let psw = "";

const rl = readline.createInterface(process.stdin, process.stdout, completer);
const help = [ ('-help                          ' + 'commanders that you can use.').magenta
           , ('-c ./json_file/create.json       ' + 'create pipeslines').magenta
           ,('-p ./json_file/property.json      ' + 'set pipeslines property').magenta
           ,('-d ./json_file/destory.json       ' + 'destory pipeslines').magenta
           , ('-q                               ' + 'exit client.').magenta
           ].join('\n');

function completer(line) {
  let completions = '-help|-c ./json_file/create.json|-p ./json_file/property.json|-d ./json_file/destory.json|-q'.split('|')
  let hits = completions.filter(function(c) {
    if (c.indexOf(line) == 0) {
      return c;
    }
  });
  return [hits && hits.length ? hits : completions, line];
} 

function prompt() {
  let arrow    = '> '
    , length = arrow.length
    ;

const rl = readline.createInterface({
 input: process.stdin, 
 output: process.stdout, 
 prompt: 'OHAI> ' }); 

const fs = require('fs')
  , filename = 'hostname.txt';
let url = 0;

function read_server_ip(){
  let array = fs.readFileSync(filename).toString().split("\n");
  array = array.filter(function(e){return e});
  console.log("HERE ARE SERVER HOSTNAME LISTS:" .yellow);
for(i in array) {
    console.log((i+" , "+array[i]).blue);
}
rl.question('Please chose server by id: '.magenta, (answer) => {
  url = array[answer];
  input_password();  
});
/*fs.readFile(filename, 'utf8', function(err, data) {
  if (err) throw err;
  console.log('found: ' + filename);
  console.log('host name is:'+ data);
  url = data;
  url= url.replace(/[\r\n]/g,"");
  set_websocket();
});*/
}

function input_password() {

  rl.question('Please input password to connect server(default: c): ', (answer) => {
    if(answer !==""){
       psw = answer;
       
       set_websocket();
    }
  });
}

//setTimeout(() => {
 //   set_websocket();
 // }, 2000);

function set_websocket(){
const ws = new WebSocket("wss://"+url+":8126/sendPath?id=2"+"&key="+psw, {
    ca: fs.readFileSync('./cert_client_8126/ca-crt.pem'),
    key: fs.readFileSync('./cert_client_8126/client1-key.pem'),
    cert: fs.readFileSync('./cert_client_8126/client1-crt.pem'),
    rejectUnauthorized:true,
    requestCert:true
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

function resume_from_pause(data) {
    rl.prompt(); //user input

    rl.on('line', (line) => { 
        input_arr = line.split(',');
        rec_pipe_arr = data.split(',');
        for(let i =0;i<rec_pipe_arr.length;i++){
            if(input_arr[0] ===rec_pipe_arr[i]){
            is_input_valid = true;
            }
        }
        switch ((line.indexOf(',')>=0) && (is_input_valid === true))
        { 
           case true: 
             console.log('right format'); 
             ws.send(line);
             is_input_valid = false;
             break; 
           default: 
             console.log(`please type right format(pipe id, p/c)`); 
             rl.close();
             ws.close();
             break; 
        } 
        rl.prompt(); // user input end
   })

   .on('close', () => { 
       console.log('[SEND_PATH_CLIENT] closed');
       process.exit(0); 
       ws.close();
    });
        
}

ws.on('open', function () {
    console.log(`[SEND_PATH_CLIENT] open()`);
    let ret = input_stream_source();
    //console.log("input_stream_source return: " + ret);
});

ws.on('message',function(data){
	console.log(`message = ${data} `);
    if(ws.readyState == WebSocket.OPEN){
        if(data.indexOf('stream source is done')>=0) {
            input_loop_times();
        } else if(data.indexOf('set loop times done')>=0) {
            input_pipe_number();
        } else if(data.indexOf(',')>-1) {
            console.log('right now pipe id is: '+ data);
            resume_from_pause(data);

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

read_server_ip();


