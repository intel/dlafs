#!/usr/bin/env node
const WebSocket = require('ws');
const readline = require('readline');
const colors = require('colors');


let create_json = "";
let property_json = "";
let destory_json = "";
let count = 0;
let pipe_constuctor = "";
let pipe_constuctor_to_number = "";
let client_id = 0;
let add_buffer ="";

let model_file_map = new Map();
for(let i=0;i<100;i++){
  model_file_map.set(i,"");
}

let model_buffer_map = new Map();
for(let i=0;i<100;i++){
  model_buffer_map.set(i,"");
}

const fs = require('fs')
  , filename = 'hostname.txt';
let url = 0;

const rl = readline.createInterface(process.stdin, process.stdout, completer);
const help = [ ('-help                          ' + 'commanders that you can use.').magenta
           ,('-c <create.json>                  ' + 'create pipeslines').magenta
           ,('-p <property.json> <pipe_id>      ' + 'set pipeslines property').magenta
           ,('-d <destroy.json>  <pipe_id>      ' + 'destroy pipeslines').magenta
           ,('-pipe                             ' + 'display pipes belonging to the very client').magenta
           ,('-client                           ' + 'display client ID').magenta
           ,('-q                                ' + 'exit client.').magenta
           ].join('\n');

function completer(line) {
  let completions = '-help|-c <create.json> |-p <property.json> <pipe_id> |-d <destory.json> <pipe_id> |-pipe |-client |-q'.split('|')
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
  if (array[answer]!== undefined){
    url = array[answer];
    set_websocket();

  }else{
    console.log("NO such server!!!please chose again" .red);
    read_server_ip();
  }
    
});
}

function set_websocket(){
const ws = new WebSocket("wss://"+url+":8126/controller", {
    ca: fs.readFileSync('./cert_client_8126_8124/ca-crt.pem'),
    key: fs.readFileSync('./cert_client_8126_8124/client1-key.pem'),
    cert: fs.readFileSync('./cert_client_8126_8124/client1-crt.pem'),
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
            jfile.client_id = parseInt(clientid);
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
            jfile.client_id = parseInt(clientid);
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
    switch (cmd[0].slice(1)) {

      case 'help':
        console.log(help.yellow);
        prompt();
      break;

      case 'c':
      read_file_sync(cmd[1],create_json,0,client_id,0);     
      break;

      case 'p':
      pipe_constuctor_to_number = pipe_constuctor.split(",");
      pipe_constuctor_to_number = pipe_constuctor_to_number.filter(function(e){return e});
      if (pipe_constuctor_to_number.includes(cmd[2]))
      {
        read_file_sync(cmd[1],property_json,2,0,cmd[2]);
          
      }else{
        console.log("Wrong command!!! please check pipe id" .red);
        prompt();
      }     
             
      break;

      case 'd':
      pipe_constuctor_to_number = pipe_constuctor.split(",");
      pipe_constuctor_to_number = pipe_constuctor_to_number.filter(function(e){return e});
      if (pipe_constuctor_to_number.includes(cmd[2]))
      {
        read_file_sync(cmd[1],destory_json,1,client_id,cmd[2]);
          
      }else{
        console.log("Wrong command!!! please check pipe id" .red);
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

function read_model_file(){

  rl.question('Please type model dictionary: '.green, (answer) => {
    if(answer !==""){
      fs.exists(answer, function(exists) {
        if (exists) {
           let i =0;
           
           fs.readdirSync(answer).forEach(file =>{
             model_file_map.set(i,file);
             console.log(file .blue);
             i++;
             
          })
          if(i===0){
            console.log("No files included in this dictionary, please check again" .red);
            read_model_file();

          }else{
            
            rl.question('All these files will be used as model files, please confirm (y/n): '.green, (choise) => {
              switch (choise){
                case 'y':
                console.log("confimed recived!" .yellow);
                
                for(let i=0;model_file_map.get(i)!=="";i++){
                  //console.log("reading");
                  let data = fs.readFileSync(answer+'/'+model_file_map.get(i));
                  //console.log(data);
                  let temp = model_file_map.get(i);
                  console.log(temp);
                  add_buffer = Buffer.alloc(1);
                  if(temp.indexOf("xml")>-1){
                    if (temp.indexOf("conf")>-1){
                      add_buffer.writeUInt8(0x3,0);
                      console.log("send conf.xml file!! " .blue);
                    }else{
                      add_buffer.writeUInt8(0x1,0);  
                      console.log("send xml file!! " .blue);
                    }

                  }else if(temp.indexOf("bin")>-1){
                    add_buffer.writeUInt8(0x2,0);
                    console.log("send bin file!! " .blue);

                  } else{
                    console.log("Controller only send files end with xml,bin!! " .red);
                  }
                  if (add_buffer[0]!==0){
                    let bu = '0x'+ client_id.toString();
                    let add_clientID = Buffer.alloc(1);
                    add_clientID.writeUInt8(bu, 0);
                    let arr = [add_clientID,add_buffer, data];
                    let buf = Buffer.concat(arr);
                    //console.log(buf);
                    //console.log(buf.slice(2));
                    //console.log(buf[0]);
                    //console.log(buf[1]);
                    ws.send(buf);
                    model_buffer_map.set(i,buf);

                  }
                  
                }
                prompt();
                rl.on('line', function(cmd) {
                    exec(cmd.trim());
                })
                //set_websocket();
                break;

                case 'n':
                console.log("please chose another dictionary" .magenta);
                read_model_file();
                break;

                default:
                console.log("wrong");
                break;

              }
            });
            
          }

          
        } else{
          console.log("dictionary NOT exist, please check again" .red);
          read_model_file();
        }
    });
  }

  
  });
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
    //read_model_file();
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

