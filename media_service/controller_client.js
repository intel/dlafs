#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');
const colors = require('colors');

let psw = "";
let create_json = "";
let property_json = "";
let destory_json = "";
let count = 0;
let pipe_constuctor = "";
let pipe_constuctor_to_number = "";
let client_id = 0;
let read_file_retur = "";

const fs = require('fs')
  , filename = 'hostname.txt';
let url = 0;

const rl = readline.createInterface(process.stdin, process.stdout, completer);
const help = [ ('-help                          ' + 'commanders that you can use.').magenta
           ,('-c <create.json>                  ' + 'create pipeslines').magenta
           ,('-p <property.json> <pipe_id>      ' + 'set pipeslines property').magenta
           ,('-d <destroy.json> <client_id> <pipe_id>      ' + 'destroy pipeslines').magenta
           ,('-pipe                             ' + 'display pipes belonging to the very client').magenta
           ,('-client                             ' + 'display client ID').magenta
           , ('-q                               ' + 'exit client.').magenta
           ].join('\n');

function completer(line) {
  let completions = '-help|-c <create.json> |-p <property.json> <pipe_id> |-d <destory.json> <client_id> <pipe_id> |-q'.split('|')
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

  function read_file_sync(path,jfile,type,clientid,pipeid){
    fs.exists(path, function(exists) {
      if (exists) {
        jfile = JSON.parse(fs.readFileSync(path, 'utf8'));
        //console.log(create_json);       
        if(jfile.command_type === type){
          switch (type) {
            case 0:
            ws.send('c' + JSON.stringify(jfile));
            console.log("Send json create command!!!" .green);
            break;
  
            case 1:
            jfile.client_id = parseInt(clientid);
            jfile.command_destroy.pipe_id = parseInt(pipeid);     
            ws.send('d' + JSON.stringify(jfile));
            console.log("Send json destory command!!!" .green);
            break;
  
            case 2:
            jfile.command_set_property.pipe_id = parseInt(pipeid);
            ws.send('p' + JSON.stringify(jfile));
            console.log("Send json set_property command!!!" .green);
            break;
  
          }
          
        }else{
          console.log("Incorrect json command!!!" .red);
          prompt();
        }
          
      }else{
          console.log("File NOT exist, please check again" .red);
          prompt();
      }
    });        
}  

function exec(command) {
   var cmd = command.split(' ');
   if (cmd[0][0] === '-') {
    //TODO: need error process but not crash
    switch (cmd[0].slice(1)) {

      case 'help':
        console.log(help.yellow);
        prompt();
      break;

      case 'c':
      read_file_sync(cmd[1],create_json,0,0,0);     
      break;

      case 'p':
      if (pipe_constuctor_to_number.includes(cmd[2]))
      {
        read_file_sync(cmd[1],property_json,2,0,cmd[2]);
          
      }else{
        console.log("Wrong command!!! please check client id and pipe id" .red);
        prompt();
      }     
             
      break;

      case 'd':
      pipe_constuctor_to_number = pipe_constuctor.split(",");
      pipe_constuctor_to_number = pipe_constuctor_to_number.filter(function(e){return e});
      if (client_id === parseInt(cmd[2]) && pipe_constuctor_to_number.includes(cmd[3]))
      {
        read_file_sync(cmd[1],destory_json,1,cmd[2],cmd[3]);
          
      }else{
        console.log("Wrong command!!! please check client id and pipe id" .red);
        prompt();
      }     
      break;

      case 'pipe':
        console.log(("Rgiht now this client owns pipes as: " + pipe_constuctor).blue);
        prompt();
      break;

      case 'client':
        console.log(('client id is '+ client_id).blue);
        prompt();
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
  //prompt();
}


ws.on('open', function () {
    console.log(`[controller] open` .yellow);
    
});

ws.on('message',function(data){
  if (count > 0){
    //console.log(count);
    //console.log(data);
    if (data.indexOf("we have killed pipe")>-1){
      console.log(`${data} ` .blue);
    }else{
      pipe_constuctor = data;
      //console.log(pipe_constuctor);
    }     
    prompt();
  }else{
    count++;
    client_id = parseInt(data);
    console.log(('client id is '+ `${data} ` ).blue); 
    prompt();
    rl.on('line', function(cmd) {
    exec(cmd.trim());
  }).on('close', function() {
    console.log('goodbye!'.green);
    process.exit(0);
  }); 
} 
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

