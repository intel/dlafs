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
           ,('-c <create.json>                  ' + 'create pipeslines').magenta
           ,('-p <property.json> <pipe_id>      ' + 'set pipeslines property').magenta
           ,('-d <destroy.json>  <pipe_id>      ' + 'destroy pipeslines').magenta
           , ('-q                               ' + 'exit client.').magenta
           ].join('\n');

function completer(line) {
  let completions = '-help|-c <create.json> |-p <property.json> <pipe_id> |-d <destory.json> <pipe_id> |-q'.split('|')
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

  rl.setPrompt(arrow.grey, length);
  rl.prompt();
}


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
  console.log('found: ' + filename .green);
  console.log('host name is:'+ data .green);
  url = data;
  url= url.replace(/[\r\n]/g,"");
  set_websocket();
});*/
}

function input_password() {

  rl.question('Please input password to connect server(default: c): '.grey, (answer) => {
    if(answer !==""){
       psw = answer;
       
       set_websocket();
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
   var cmd = command.split(' ');
   var num = cmd.length;
   if (cmd[0][0] === '-') {
    //TODO: need error process but not crash
    switch (cmd[0].slice(1)) {
      case 'help':
        console.log(help.yellow);
        break;
      case 'c':
        if(fs.existsSync(cmd[1])) {
            create_json = JSON.parse(fs.readFileSync(cmd[1], 'utf8'));
            console.log("Send json create command!!!");
            if(create_json.command_type==0)
                ws.send('c' + JSON.stringify(create_json));
            else
                console.log("Incorrect json create command!!!");
        } else {
            console.log(cmd[1] + " doesn't exist!!!");
        }
        break;
      case 'p':
        if(fs.existsSync(cmd[1])) {
            property_json = JSON.parse(fs.readFileSync(cmd[1], 'utf8'));
            if(num==3)
                property_json.command_set_property.pipe_id = parseInt(cmd[2]);
            console.log("Send json set_property command!!!");
            if(property_json.command_type==2)
                ws.send('p' + JSON.stringify(property_json));
            else
                console.log("Incorrect json set_property command!!!");
        }
        break;
      case 'd':
        if(fs.existsSync(cmd[1])) {
            destroy_json = JSON.parse(fs.readFileSync(cmd[1], 'utf8'));
            if(num==3)
                destroy_json.command_destroy.pipe_id = parseInt(cmd[2]);
            console.log('cmd[2] =', destroy_json.command_destroy.pipe_id);
            console.log("Send json destroy command!!!");
            if(destroy_json.command_type==1)
                ws.send('d' + JSON.stringify(destroy_json));
            else
                console.log("Incorrect json destroy command!!!");
        }
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

read_server_ip();


