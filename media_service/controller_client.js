#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');
const colors = require('colors');

let psw = "";
let create_json = "";
let property_json = "";
let destory_json = "";

const fs = require('fs')
  , filename = 'hostname.txt';
let url = 0;

const rl = readline.createInterface(process.stdin, process.stdout, completer);
const help = [ ('-help                          ' + 'commanders that you can use.').magenta
           , ('-c ./json_file/create.json       ' + 'create pipeslines').magenta
           ,('-p ./json_file/property.json      ' + 'set pipeslines property').magenta
           ,('-d ./json_file/destory.json       ' + 'destory pipeslines').magenta
           , ('-q                               ' + 'exit client.').magenta
           ].join('\n');

function completer(line) {
  var completions = '-help|-c ./json_file/create.json|-p ./json_file/property.json|-d ./json_file/destory.json|-q'.split('|')
  var hits = completions.filter(function(c) {
    if (c.indexOf(line) == 0) {
      return c;
    }
  });
  return [hits && hits.length ? hits : completions, line];
} 

function prompt() {
  var arrow    = '> '
    , length = arrow.length
    ;

  rl.setPrompt(arrow.grey, length);
  rl.prompt();
}


function read_server_ip(){
fs.readFile(filename, 'utf8', function(err, data) {
  if (err) throw err;
  console.log('found: ' + filename .green);
  console.log('host name is:'+ data .green);
  url = data;
  url= url.replace(/[\r\n]/g,"");
  set_websocket();
});
}

function input_password() {

  rl.question('Please input password to connect server(default: c): '.grey, (answer) => {
    if(answer !==""){
       psw = answer;
       read_server_ip();
    }
  });
}

function set_websocket(){
const ws = new WebSocket("wss://"+url+":8126/controller?id=2"+"&key="+psw, {
    ca: fs.readFileSync('./cert_client_8126/ca-crt.pem'),
    key: fs.readFileSync('./cert_client_8126/client1-key.pem'),
    cert: fs.readFileSync('./cert_client_8126/client1-crt.pem'),
    rejectUnauthorized:true,
    requestCert:true
  });

function exec(command) {
   if (command[0] === '-') {
  
    switch (command.slice(1)) {
      case 'help':
        console.log(help.yellow);
        break;
      case 'c ./json_file/create.json':
        create_json = JSON.parse(fs.readFileSync('./json_file/create.json', 'utf8'));
        ws.send('c' + JSON.stringify(create_json));
        break;
      case 'p ./json_file/property.json':
        property_json = JSON.parse(fs.readFileSync('./json_file/property.json', 'utf8'));
        ws.send('p' + JSON.stringify(property_json));
        break;
      case 'd ./json_file/destory.json':
        destory_json = JSON.parse(fs.readFileSync('./json_file/destory.json', 'utf8'));
        ws.send('d' + JSON.stringify(destory_json));
        break;
      case 'q':
        process.exit(0);
        break;
    }
  } else {
    // only print if they typed something
    if (command !== '') {
      console.log(('\'' + command 
                  + '\' is not a command').red);
    }
  }
  prompt();
}


ws.on('open', function () {
    console.log(`[controller] open` .yellow);
});

ws.on('message',function(data){
  console.log(`${data} ` .blue);
  prompt();
  rl.on('line', function(cmd) {
  exec(cmd.trim());
}).on('close', function() {
  console.log('goodbye!'.green);
  process.exit(0);
});
    
});


ws.on('error', function (err) {
    console.log(`connect wrong!` .red);
    console.log(err);
}); 

ws.on('close', function () {
    console.log(`Right now I'll clear all pipes!` .yellow);
});
}

input_password();

