#!/usr/bin/env node
//Copyright (C) 2018 Intel Corporation
// 
//SPDX-License-Identifier: MIT
//
//MIT License
//
//Copyright (c) 2018 Intel Corporation
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation the rights to
//use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
//AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

const WebSocket = require('ws');
const readline = require('readline');
const colors = require('colors');
const crypto = require('crypto');


let create_json = "";
let property_json = "";
let destory_json = "";
let count = 0;
let pipe_constuctor = "";
let pipe_constuctor_to_number = "";
let client_id = 0;
let receive_model_file_info = "";
let show_file = 0;
let show_interface = 0;
let bin_md5 = "";
let xml_md5 = "";
let conf_md5 = "";
let send_file_name = "";
let model_file_json = "";
let file_count = 0;
let created_json_string = {} // empty Object
let flag = false;


let model_file_map = new Map();
for (let i = 0; i < 100; i++) {
  model_file_map.set(i, "");
}

let model_buffer_map = new Map();
for (let i = 0; i < 100; i++) {
  model_buffer_map.set(i, "");
}

let model_hash_map = new Map();
for (let i = 0; i < 100; i++) {
  model_hash_map.set(i, "");
}

const fs = require('fs')
  , filename = 'hostname.txt';
let url = 0;


function checksum(str, algorithm, encoding) {
  return crypto
    .createHash(algorithm || 'md5')
    .update(str, 'utf8')
    .digest(encoding || 'hex')
}

function display_json(data) {
  let i = 0;
  console.log("HERE ARE SERVER MODEL LISTS:".yellow);
  for (key in data) {
    console.log((i + '. ' + key).bgBlue);
    i++;
    for (innerKey in data[key]) {
      console.log(innerKey + ":     " + JSON.stringify(data[key][innerKey]).yellow);

    }
    console.log("\r\n");
  }
}

const rl = readline.createInterface(process.stdin, process.stdout, completer);
const help = [('-help                          ' + 'commanders that you can use.').magenta
  , ('-c <create.json>                  ' + 'create pipeslines').magenta
  , ('-p <property.json> <pipe_id>      ' + 'set pipeslines property').magenta
  , ('-d <destroy.json>  <pipe_id>      ' + 'destroy pipeslines').magenta
  , ('-pipe                             ' + 'display pipes belonging to the very client').magenta
  , ('-client                           ' + 'display client ID').magenta
  , ('-model                            ' + 'display model info').magenta
  , ('-q                                ' + 'exit client.').magenta
].join('\n');

function completer(line) {
  let completions = '-help|-c <create.json> |-p <property.json> <pipe_id> |-d <destory.json> <pipe_id> |-pipe |-client |-model |-q'.split('|')
  let hits = completions.filter(function (c) {
    if (c.indexOf(line) == 0) {
      return c;
    }
  });
  return [hits && hits.length ? hits : completions, line];
}

function prompt() {
  let arrow = '> '
    , length = arrow.length
    ;

  rl.setPrompt(arrow.grey, length);
  rl.prompt();
}


function read_server_ip() {
  let array = fs.readFileSync(filename).toString().split("\n");
  array = array.filter(function (e) { return e });
  console.log("HERE ARE SERVER HOSTNAME LISTS:".yellow);
  for (i in array) {
    console.log((i + " , " + array[i]).blue);
  }
  rl.question('Please chose server by id: '.magenta, (answer) => {
    if (array[answer] !== undefined) {
      url = array[answer];
      set_websocket();

    } else {
      console.log("NO such server!!!please chose again".red);
      read_server_ip();
    }

  });
}

function set_websocket() {
  const ws = new WebSocket("wss://" + url + ":8126/controller", {
    ca: fs.readFileSync('./cert_client_8126_8124/ca-crt.pem'),
    key: fs.readFileSync('./cert_client_8126_8124/client1-key.pem'),
    cert: fs.readFileSync('./cert_client_8126_8124/client1-crt.pem'),
    rejectUnauthorized: true,
    requestCert: true
  });

  function isEmpty(obj) {
    for (var key in obj) {
      if (obj.hasOwnProperty(key))
        return false;
    }
    return true;
  }

  function check_json_file(jsonfile, id) {
    let vali = true;
    if (isEmpty(jsonfile)) {
      vali = false;
      // console.log("Empty object existed in json file".red);
    } else {
      switch (id) {
        case 0:
          if (jsonfile.command_create.pipe_number < 1) {
            vali = false;
            console.log("Please check pipe number in json file".red);
          } else if (jsonfile.command_create.stream_source === "") {
            vali = false;
            console.log("Please check stream source in json file".red);

          } else if (jsonfile.command_create.loop_times < 1) {
            vali = false;
            console.log("Please check loop times in json file".red);
          }
          break;

        case 1:
          vali = true;
          break;

        case 2:
          if (jsonfile.command_set_property.cvdlfilter0.algopipeline === "") {
            vali = false;
            console.log("Please check algopipeline in json file".red);
          }
          break;
      }
    }
    return vali;

  }

  function read_file_sync(path, jfile, type, clientid, pipeid) {
    fs.exists(path, function (exists) {
      if (exists) {
        try {
          jfile = JSON.parse(fs.readFileSync(path, 'utf8'));
        } catch (e) {
          console.log("Json file error!!! Please fill empty object".red);
          //return console.error(e);
          prompt();
        }
        //console.log(create_json); 
        if (check_json_file(jfile, type)) {
          if (jfile.command_type === type) {
            switch (type) {
              case 0:
                jfile.client_id = parseInt(clientid);
                ws.send('c' + JSON.stringify(jfile));
                console.log("Send json create command!!!".green);
                break;

              case 1:
                jfile.client_id = parseInt(clientid);
                jfile.command_destroy.pipe_id = parseInt(pipeid);
                ws.send('d' + JSON.stringify(jfile));
                console.log("Send json destory command!!!".green);
                break;

              case 2:
                jfile.client_id = parseInt(clientid);
                jfile.command_set_property.pipe_id = parseInt(pipeid);
                ws.send('p' + JSON.stringify(jfile));
                console.log("Send json set_property command!!!".green);
                break;

            }

          } else {
            console.log("Incorrect json command!!!".red);

          }
        } else {
          prompt();

        }

      } else {
        console.log("File NOT exist, please check again".red);
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
          read_file_sync(cmd[1], create_json, 0, client_id, 0);
          break;

        case 'p':
          pipe_constuctor_to_number = pipe_constuctor.split(",");
          pipe_constuctor_to_number = pipe_constuctor_to_number.filter(function (e) { return e });
          if (pipe_constuctor_to_number.includes(cmd[2])) {
            read_file_sync(cmd[1], property_json, 2, 0, cmd[2]);

          } else {
            console.log("Wrong command!!! please check pipe id".red);
            prompt();
          }

          break;

        case 'd':
          pipe_constuctor_to_number = pipe_constuctor.split(",");
          pipe_constuctor_to_number = pipe_constuctor_to_number.filter(function (e) { return e });
          if (pipe_constuctor_to_number.includes(cmd[2])) {
            read_file_sync(cmd[1], destory_json, 1, client_id, cmd[2]);

          } else {
            console.log("Wrong command!!! please check pipe id".red);
            prompt();
          }
          break;

        case 'pipe':
          if (pipe_constuctor === "") {
            console.log("There are NO pipes owned by this client".red);
          } else {
            console.log(("Rgiht now this client owns pipes as: " + pipe_constuctor).blue);
          }
          prompt();
          break;

        case 'client':
          console.log(('client id is ' + client_id).blue);
          prompt();
          break;

        case 'model':
          //console.log(JSON.stringify(model_file, null, 4));
          let print_model = JSON.parse(receive_model_file_info);
          //console.log(JSON.stringify(print_model));
          //console.log(JSON.stringify(print_model, null, "  "));
          display_json(print_model);
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

  function compare_MD5(model_information) {
    if (model_information[send_file_name].model_file.bin_file.MD5 === bin_md5) {
      if (model_information[send_file_name].model_file.xml_file.MD5 === xml_md5) {
        if (model_information[send_file_name].model_file.conf_file.MD5 === conf_md5) {
          console.log("Server has received your models files".blue);
          prompt();
          rl.on('line', function (cmd) {
            exec(cmd.trim());
          });
        } else {
          console.log("Conf file send error!!!Please check again!!!".red);
          read_model_file();
        }
      } else {
        console.log("Xml file send error!!!Please check again!!!".red);
        read_model_file();
      }
    } else {
      console.log("Bin file send error!!!Please check again!!!".red);
      read_model_file();
    }
  }

  function create_json_object(key, data, file_type, md5_value) {
    switch (file_type) {
      case 'bin':
        created_json_string[key]["bin_file"] = {};
        created_json_string[key]["bin_file"]["name"] = data;
        created_json_string[key]["bin_file"]["MD5"] = md5_value;
        break;

      case 'xml':
        created_json_string[key]["xml_file"] = {};
        created_json_string[key]["xml_file"]["name"] = data;
        created_json_string[key]["xml_file"]["MD5"] = md5_value;
        break;

      case 'conf':
        created_json_string[key]["conf_file"] = {};
        created_json_string[key]["conf_file"]["name"] = data;
        created_json_string[key]["conf_file"]["MD5"] = md5_value;
        break;
    }

  }


  function read_model_file() {
    rl.question('Please type model dictionary: '.green, (answer) => {
      if (answer !== "") {
        fs.exists(answer, function (exist) {
          if (exist) {
            fs.readdirSync(answer).forEach(subdirs => {
              //console.log(subdirs);

              if (fs.lstatSync(answer + "/" + subdirs).isDirectory()) {
                created_json_string[subdirs] = {};
                fs.readdirSync(answer + "/" + subdirs).forEach(file => {
                  let i = 0;
                  model_file_map.set(i, file);
                  console.log(file.blue);
                  //console.log("Analyzing your model......".yellow);
                  //send_file = answer.lastIndexOf("/");
                  //send_file_name = answer.substring(send_file + 1);
                  send_file_name = subdirs;
                  console.log(subdirs);
                  if (receive_model_file_info.indexOf(send_file_name) > -1) {
                    model_file_json = JSON.parse(receive_model_file_info);
                    //console.log(crypto.createHash("md5").update(file).digest("hex"));
                    //console.log(model_file_json[send_file_name].model_file.bin_file.MD5);
                    if (file.indexOf("bin") > -1) {
                      bin_md5 = crypto.createHash("md5").update(file).digest("hex");
                      if (bin_md5 === (model_file_json[send_file_name].model_file.bin_file.MD5)) {
                        console.log("server already has this model file".red);
                      } else {
                        console.log("updating this file in server".green);
                        let data = fs.readFileSync(answer + '/' + subdirs + "/" + file);
                        let buf = Buffer.from(data);
                        ws.send("m" + send_file_name + "/" + file);
                        ws.send(buf);
                        flag = true;
                        file_count++;
                        create_json_object(subdirs, file, 'bin', bin_md5);
                      }
                    } else if (file.indexOf("xml") > -1) {
                      if (file.indexOf("conf") > -1) {
                        conf_md5 = crypto.createHash("md5").update(file).digest("hex");
                        if (conf_md5 === (model_file_json[send_file_name].model_file.conf_file.MD5)) {
                          console.log("server already has this model file".red);

                        } else {
                          console.log("updating this file in server".green);
                          let data = fs.readFileSync(answer + '/' + subdirs + "/" + file);
                          let buf = Buffer.from(data);
                          ws.send("m" + send_file_name + "/" + file);
                          ws.send(buf);
                          flag = true;
                          file_count++;
                          create_json_object(subdirs, file, 'conf', conf_md5);
                        }

                      } else {
                        xml_md5 = crypto.createHash("md5").update(file).digest("hex");
                        if (xml_md5 === (model_file_json[send_file_name].model_file.xml_file.MD5)) {
                          console.log("server already has this model file".red);

                        } else {
                          console.log("updating this file in server".green);
                          let data = fs.readFileSync(answer + '/' + subdirs + "/" + file);
                          let buf = Buffer.from(data);
                          ws.send("m" + send_file_name + "/" + file);
                          ws.send(buf);
                          flag = true;
                          file_count++;
                          create_json_object(subdirs, file, 'xml', xml_md5);
                        }

                      }

                    }

                    i++;
                  } else {
                    let data = fs.readFileSync(answer + '/' + subdirs + "/" + file);
                    if (file.indexOf("bin") > -1) {
                      bin_md5 = crypto.createHash("md5").update(file).digest("hex");
                      file_count++;
                      let buf = Buffer.from(data);
                      ws.send("m" + send_file_name + "/" + file);
                      ws.send(buf);
                      flag = true;
                      create_json_object(subdirs, file, 'bin', bin_md5);
                    } else if (file.indexOf("xml") > -1) {
                      if (file.indexOf("conf") > -1) {
                        conf_md5 = crypto.createHash("md5").update(file).digest("hex");
                        file_count++;
                        let buf = Buffer.from(data);
                        ws.send("m" + send_file_name + "/" + file);
                        ws.send(buf);
                        flag = true;
                        create_json_object(subdirs, file, 'conf', conf_md5);
                      } else {
                        xml_md5 = crypto.createHash("md5").update(file).digest("hex");
                        file_count++;
                        let buf = Buffer.from(data);
                        ws.send("m" + send_file_name + "/" + file);
                        ws.send(buf);
                        flag = true;
                        create_json_object(subdirs, file, 'xml', xml_md5);
                      }
                    } else {
                      console.log("this is not xml or bin file".red);
                    }
                  }

                })

                if (i === 0) {
                  console.log("No files included in this dictionary, please check again".red);
                  read_model_file();
                }

              } else {
                console.log("This is not dictionary".red);
                //console.log("dictionary NOT exist, please check again".red);
                //read_model_file();
              }
            });
            ws.send("file numbers are:" + file_count);
            //console.log(file_count);
            if (flag === true) {
              ws.send("file MD5 are:" + JSON.stringify(created_json_string));
            } else {
              prompt();
              rl.on('line', function (cmd) {
                exec(cmd.trim());
              });
            }
            file_number = file_count;
            //console.log(created_json_string);
          } else {
            console.log("dictionary NOT exist, please check again".red);
            read_model_file();
          }
        });
      }


    });
  }


  ws.on('open', function () {
    console.log(`[controller] open`.yellow);

  });

  ws.on('message', function (data) {
    if (count > 0) {
      if (data.indexOf("we have killed pipe") > -1) {
        console.log(`${data} `.blue);
        prompt();
      } else if (data.indexOf("{") > -1) {
        receive_model_file_info = data;
        if (show_file === 0) {
          display_json(JSON.parse(data));
          //console.log(JSON.parse(data));
          show_file++;
          read_model_file();
        }
        //TODO: Parse client ID to decide whether start intherface or not, server also need !!!!!
      } else if (data.indexOf("models update finished") > -1) {
        //console.log("show_interface is " +show_interface);  
        // if(show_interface ===0){ 
        console.log("Server has updated its model, use -model to check".blue);
        // show_interface++;
        //console.log(JSON.stringify(JSON.parse(receive_model_file_info)));
        prompt();
        rl.on('line', function (cmd) {
          exec(cmd.trim());
        });
        //}
      } else if (data.indexOf("Server models updated") > -1) {
        console.log("Server has updated its model, use -model to check".blue);
        prompt();
      } else if (data.indexOf("has finished and exit!") > -1) {
        console.log(`${data} `.blue);
        prompt();
        //compare_MD5(JSON.parse(receive_model_file_info));
      } else {
        pipe_constuctor = data;
        prompt();
      }

    } else {
      count++;
      client_id = parseInt(data);
      console.log(('client id is ' + `${data} `).blue);
    }
  });



  ws.on('error', function (err) {
    console.log(`connect wrong!`.red);
    console.log(err);
  });

  ws.on('close', function () {
    console.log(`Right now I'll clear all pipes!`.yellow);
  });
}

read_server_ip();

