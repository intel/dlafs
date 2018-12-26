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

"use strict";
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const fs = require('fs');
const https = require('https');
const spawn = require('child_process').spawn;
const colors = require('colors');
const crypto = require('crypto');

let client_id = 0;
let pipe_id = 0;
let create_json = "";
let destory_json = "";
let property_json = "";
let pipe_count = 0;
let client_pipe = "";
let per_client_pipe = "";
let ws_index = 0;
let temp_json_path = "";
let model_path = "";
let model_name = "";
let file_name = "";
let model_file = "";
let file_number = 0;
let receiver_count = 0;
let file_count = 0;
let count = 0;
let model_updateing_controller_client = "";
let received_md5 = "";


let controller_map = new Map();
for (let i = 0; i < 100; i++) {
  controller_map.set(i, 0);
}

let pipe_map = new Map();
for (let i = 0; i < 100; i++) {
  pipe_map.set(i, 0);
}

let client_map = new Map();
for (let i = 0; i < 100; i++) {
  client_map.set(i, "");
}

function read_file_number(dir) {
  fs.readdir(dir, (err, files) => {
    if (err) {
      console.log(err);
    }
    return files.length;
  });
}

function read_file(dir) {
  let model_files = "";
  fs.readdirSync(dir).forEach(file => {
    model_files = model_files + file + ',';
    return model_files;
  });
}

function checksum(str, algorithm, encoding) {
  return crypto
    .createHash(algorithm || 'md5')
    .update(str, 'utf8')
    .digest(encoding || 'hex')
}

function create_directionary() {
  let dire_name = ['yolov1tiny', 'googlenetv2', 'mobilenetssd', 'lprnet', 'yolov2tiny', 'reid'];
  for (let i = 0; i < dire_name.length; i++) {
    try {
      fs.accessSync('./models/' + dire_name[i], fs.constants.F_OK);
    } catch (ex) {
      if (ex) {
        fs.mkdir('./models/' + dire_name[i], function (err) {
          if (err) {
            throw err;
          }
        });
      }
    }
  }
}


const controller_server = https.createServer({
  key: fs.readFileSync('./cert_server_8126_8124/server-key.pem'),
  cert: fs.readFileSync('./cert_server_8126_8124/server-crt.pem'),
  ca: fs.readFileSync('./cert_server_8126_8124/ca-crt.pem'),
  crl: fs.readFileSync('./cert_client_8126_8124/client.crl'),
  requestCert: true,
  rejectUnauthorized: true
});

const controller_wss = new WebSocketServer({ server: controller_server, path: '/controller' });

controller_wss.on('connection', function (ws) {

  console.log('controller connected !'.bgYellow);
  create_directionary();
  client_id++;
  file_count = 0;
  ws.send(client_id);
  controller_map.set(client_id, ws);
  model_file = fs.readFileSync('./models/model_info.json', 'utf8');
  ws.send(model_file);

  // Broadcast to all.
  //TODO: Add client ID to broadcast
  controller_wss.broadcast = function broadcast(msg, updating_client) {
    controller_wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        client.send(msg);
        if (client === updating_client) {
          client.send("models update finished");
        } else {
          client.send("Server models updated");
        }
        console.log("send updated to one of your client".bgGreen);
      }
    });
  };


  function update_model_info(model_updating_client) {
    console.log("models updating...");
    let model_update = JSON.parse(fs.readFileSync('./models/model_info.json', 'utf8'));
    //console.log(model_update);
    count = 0;
    for (let key in received_md5) {
      let cou = 0;
      let temp = "";
      if (fs.lstatSync('./models/' + key).isDirectory()) {
        for (let innerkey in received_md5[key]) {
          //console.log(received_md5[key][innerkey]);
          switch (innerkey) {
            case "bin_file":
              fs.readFile('./models/' + key + '/' + received_md5[key].bin_file.name, function (err) {
                let data = fs.readFileSync('./models/' + key + '/' + received_md5[key].bin_file.name);
                temp = received_md5[key].bin_file.name;
                model_update[key].model_file.bin_file.MD5 = crypto.createHash("md5").update(data).digest("hex");
                if (model_update[key].model_file.bin_file.MD5 === received_md5[key].bin_file.MD5) {
                  model_update[key].model_file.bin_file.name = temp;
                  cou++;
                  count++;
                  //console.log(count);
                  //console.log(cou);
                  if (cou > 1) {
                    model_update[key].has_model = 'Yes';
                  }
                  console.log("Bin file updated successfully!!!".green);
                  fs.writeFileSync('./models/model_info.json', JSON.stringify(model_update));
                  console.log("count:" + count + "    " + "file_number:" + file_number);
                  if (count === file_number) {
                    model_file = fs.readFileSync('./models/model_info.json', 'utf8');
                    controller_wss.broadcast(model_file, model_updating_client);
                    //controller_wss.broadcast("models update finished");
                    console.log("send json file!!!".red);
                  }
                } else {
                  console.log("Bin file send error".red);
                }
                if (err) {
                  console.log("File not exist!!!".red);
                  throw err;
                }
              });

              break;

            case "xml_file":
              fs.readFile('./models/' + key + '/' + received_md5[key].xml_file.name, function (err) {
                let data = fs.readFileSync('./models/' + key + '/' + received_md5[key].xml_file.name);
                temp = received_md5[key].xml_file.name;
                model_update[key].model_file.xml_file.MD5 = crypto.createHash("md5").update(data).digest("hex");
                if (model_update[key].model_file.xml_file.MD5 === received_md5[key].xml_file.MD5) {
                  model_update[key].model_file.xml_file.name = temp;
                  cou++;
                  count++;
                  //console.log(count);
                  //console.log(cou);
                  if (cou > 1) {
                    model_update[key].has_model = 'Yes';
                  }
                  console.log("Xml file updated successfully!!!".green);
                  fs.writeFileSync('./models/model_info.json', JSON.stringify(model_update));
                  console.log("count:" + count + "    " + "file_number:" + file_number);
                  if (count === file_number) {
                    model_file = fs.readFileSync('./models/model_info.json', 'utf8');
                    controller_wss.broadcast(model_file, model_updating_client);
                    console.log("send json file!!!".red);
                  }
                } else {
                  console.log("Xml file send error".red);
                }
                if (err) {
                  console.log("File not exist!!!".red);
                  throw err;
                }
              });

              break;

            case "conf_file":
              fs.readFile('./models/' + key + '/' + received_md5[key].conf_file.name, function (err) {
                let data = fs.readFileSync('./models/' + key + '/' + received_md5[key].conf_file.name);
                temp = received_md5[key].conf_file.name;
                model_update[key].model_file.conf_file.MD5 = crypto.createHash("md5").update(data).digest("hex");
                if (model_update[key].model_file.conf_file.MD5 === received_md5[key].conf_file.MD5) {
                  model_update[key].model_file.conf_file.name = temp;
                  count++;
                  //console.log(count);
                  console.log("Conf file updated successfully!!!".green);
                  fs.writeFileSync('./models/model_info.json', JSON.stringify(model_update));
                  //console.log("count:" + count + "    " + "file_number:" + file_number);
                  if (count === file_number) {
                    model_file = fs.readFileSync('./models/model_info.json', 'utf8');
                    controller_wss.broadcast(model_file, model_updating_client);
                    console.log("send json file!!!".red);
                  }
                } else {
                  console.log("Conf file send error".red);
                }
                if (err) {
                  console.log("File not exist!!!".red);
                  throw err;
                }

              });

              break;
          }

        }

      } else {
        console.log("Dictionary not exist!!!".red);
      }
    }

  }


  ws.on('message', function (path) {

    //console.log("receive message:" + path);
    //console.log(typeof path);    
    if (typeof path === 'string') {
      if (path.indexOf("file MD5 are") > -1) {
        let length = path.indexOf(":");
        received_md5 = JSON.parse(path.substring(length + 1));
        console.log(received_md5);
        //broadcast_to_all();
        //console.log(received_md5);
      } else if (path.indexOf("file numbers are") > -1) {
        let temp = path.indexOf(":");
        file_number = parseInt(path.substring(temp + 1));
      }
      else {

        switch (path[0]) {
          case 'c':
            create_json = JSON.parse(path.substring(1));
            if (client_map.get(create_json.client_id) === "") {
              client_pipe = "";

            } else {
              client_pipe = client_map.get(create_json.client_id);
            }

            for (let i = 0; i < create_json.command_create.pipe_num; i++) {
              gst_cmd = 'hddlspipes -i  ' + pipe_count + ' -l ' + create_json.command_create.loop_times;

              let child = spawn(gst_cmd, {
                shell: true
              });

              child.stderr.on('data', function (data) {
                console.error("STDERR:", data.toString());
              });

              child.stdout.on('data', function (data) {
                console.log("STDOUT:", data.toString());
              });

              child.on('exit', function (exitCode) {
                console.log("Child exited with code: " + exitCode);
              });

              client_pipe = client_pipe + pipe_count.toString() + ",";
              pipe_id = pipe_count;


              temp_json_path = './client_' + create_json.client_id + '_temp_create.json';
              fs.writeFile(temp_json_path, JSON.stringify(create_json), { flag: 'w' }, function (err) {
                if (err) {
                  console.error("write file failed: ", err);
                } else {
                  console.log('temp json: done');
                }
              });
              //console.log(pipe_id);
              //console.log(pipe_count);
              pipe_count++;

            }

            client_map.set(create_json.client_id, client_pipe);
            //console.log(client_map);
            ws.send(client_map.get(create_json.client_id));
            break;

          case 'p':
            property_json = JSON.parse(path.substring(1));
            ws_index = pipe_map.get(property_json.command_set_property.pipe_id);
            ws_index.send(path.substring(1));
            break;

          case 'd':
            destory_json = JSON.parse(path.substring(1));
            ws_index = pipe_map.get(destory_json.command_destroy.pipe_id);
            //console.log(ws_index);
            //ws_index.close();
            console.log(path.substring(1));
            ws_index.send(path.substring(1));
            //console.log(pipe_map);
            per_client_pipe = client_map.get(destory_json.client_id);
            per_client_pipe = per_client_pipe.replace(destory_json.command_destroy.pipe_id.toString() + ",", "");
            client_map.set(destory_json.client_id, per_client_pipe);
            //console.log(client_map);
            console.log('we killed pipe ' + destory_json.command_destroy.pipe_id);
            pipe_map.set(destory_json.command_destroy.pipe_id, -1);
            ws.send("we have killed pipe " + destory_json.command_destroy.pipe_id);
            console.log("we have send to client".bgRed);
            ws.send(client_map.get(destory_json.client_id));
            break;

          case 'm':
            let length = path.lastIndexOf("/");
            //console.log(length);
            model_name = path.substring(1, length);
            file_name = path.substring(length + 1);
            console.log(model_name);
            console.log(file_name);
            break;
        }
      }

    } else if (typeof path === 'object') {
      console.log("I received buffer");
      let buff = Buffer.from(path);
      let model_dir_path = './models/' + model_name;
      let temp = model_dir_path.lastIndexOf("/");
      let dire = model_dir_path.substring(temp + 1);
      console.log(("output dir: ", model_dir_path).bgMagenta);
      try {
        fs.accessSync(model_dir_path, fs.constants.F_OK);
      } catch (ex) {
        console.log(("access error for ", model_dir_path).red);
        if (ex) {
          console.log(("please build a new directory for ", model_dir_path).red);
          fs.mkdir(model_dir_path, function (err) {
            if (err) {
              console.log(("Failed to create dir, err =  ", err).red);
            }
            let model_update = JSON.parse(fs.readFileSync('./models/model_info.json', 'utf8'));
            model_update[dire] = {
              "has_model": "No", "model_file":
              {
                "bin_file": { "name": "", "MD5": "" },
                "xml_file": { "name": "", "MD5": "" },
                "conf_file": { "name": "", "MD5": "" }
              }
            };
            console.log(dire.bgBlue);
            fs.writeFileSync('./models/model_info.json', JSON.stringify(model_update));
            console.log("write down");
          });
        } else {
          console.log(("output data will be put into ", model_dir_path).green);
          return;
        }
      };

      model_path = model_dir_path + '/' + file_name;


      console.log(buff);
      //let fd = fs.openSync(model_path, 'w');
      fs.open(model_path, 'w', function (err, fd) {
        if (err) throw err;
        //console.log('Saved!');
        fs.write(fd, buff, 0, buff.length, null, function (err) {
          if (err) throw 'error writing file: ' + err;
          fs.close(fd, function (errs) {
            console.log('file written');
            file_count++;
            console.log(file_count);
            if (file_count === parseInt(file_number)) {
              model_updateing_controller_client = ws;
              update_model_info(model_updateing_controller_client);
            }
          })
          if (err) {
            throw err;
          }
        });
      });

    }
  });



  ws.on('close', function () {
    console.log('controller closed!'.bgYellow);
  });

  ws.on('error', function (e) {
    console.log('controller connection error!'.bgRed);
    console.log(e);
  });


});

//const util = require('util');
const url = require('url');
let params = 0;
let userArray = [];
let receive_client = 0;
let gst_cmd = 0;
let pipe_client = 0;



const hddlpipe_server = https.createServer({
  cert: fs.readFileSync('./cert_server_8123/server-cert.pem'),
  key: fs.readFileSync('./cert_server_8123/server-key.pem'),
  strictSSL: false
});

const hddlpipe_wss = new WebSocketServer({ server: hddlpipe_server, path: '/binaryEchoWithSize' });

hddlpipe_wss.on('connection', function connection(ws) {

  console.log("Local socket connected!!!".bgGreen);
  fs.readFile(temp_json_path, 'utf8', function (err, data) {
    if (err) throw err;
    console.log("read create.config: ", data);
    ws.send(data);
  });

  userArray.push(ws);
  console.log("this is " + userArray.indexOf(ws) + "th loop time");


  function update_pipe_info() {
   // console.log("I know you have closed!!!" .red);
    // console.log(pipe_map);
    //console.log(pipe_map.length());
    //let temp = pipe_map.get(i);
    //console.log(temp);
    for (let i = 0; i < 100; i++) {
      if (pipe_map.get(i) !== 0 && pipe_map.get(i) !== -1 && pipe_map.get(i).readyState === WebSocket.CLOSED) {
        pipe_map.set(i, -1);
       // console.log("I know you have closed!!!" .blue);
        //console.log(client_map);
      //  console.log("i is" + i);
        for (let j = 0; j < 100; j++) {
          if (client_map.get(j) !== "" && client_map.get(j).indexOf(i.toString()) > -1) {
            //console.log(client_map.get(j).indexOf(i.toString()));
            //console.log("j is" + j);
            let temple = client_map.get(j).replace(i.toString() + ",", "");
            //console.log(temple);
            client_map.set(j, temple);
            if (controller_map.get(j).readyState === WebSocket.OPEN) {
              controller_map.get(j).send("pipe " + i + " has finished and exit!");
              controller_map.get(j).send(temple);
            }
          }
        }
      }
    }
  }


  ws.on('message', function incoming(data) {
    if (typeof data === "string") {
      //console.log(data);
      pipe_client = data.split("=");
      // console.log(pipe_client);
      pipe_id = parseInt(pipe_client[1]);
      //console.log(pipe_id);
      pipe_map.set(pipe_id, ws);
      //console.log(pipe_map);
    }
    else {
      if (receive_client.readyState === WebSocket.OPEN) {

        receive_client.send(data);
      } else {
        console.log("Please open receiver client!".red);
      }
    }
  });

  ws.on('error', function (e) {
    console.log('receiver connection error!'.bgRed);
    console.log(e);
  });

  ws.on('close', function () {
   // console.log("Why you are still here!!!!".bgRed);
    console.log("pipeline closed!".bgMagenta);
    update_pipe_info();
  });

});

function ClientVerify(info) {

  let ret = false;//refuse
  if (receiver_count === 0) {
    ret = true;
  }
  return ret;
}


const receiver_server = https.createServer({
  key: fs.readFileSync('./cert_server_8126_8124/server-key.pem'),
  cert: fs.readFileSync('./cert_server_8126_8124/server-crt.pem'),
  ca: fs.readFileSync('./cert_server_8126_8124/ca-crt.pem'),
  crl: fs.readFileSync('./cert_client_8126_8124/client.crl'),
  requestCert: true,
  rejectUnauthorized: true
});

const receiver_wss = new WebSocketServer({ server: receiver_server, path: '/routeData', verifyClient: ClientVerify });
receiver_wss.on('connection', function connection(ws) {
  receiver_count++;
  console.log("Receiver connected!".bgMagenta);
  receive_client = ws;

  ws.on('error', function (e) {
    console.log('receiver connection error!'.bgRed);
    console.log(e);

  });

  ws.on('close', function (ws) {
    receiver_count--;
    console.log("Receiver closed!".bgMagenta);
  });


});


controller_server.listen(8126);
hddlpipe_server.listen(8123);
receiver_server.listen(8124);
process.on('uncaughtException', function (err) {
  if (err.errno === 'EADDRINUSE')
    console.log("Address already in use!!! Please check if server is already running".red);
  else
    console.log(err);
  process.exit(1);
});
console.log('Listening on port 8126, 8124 and 8213...'.rainbow);

const exec = require('child_process').exec;
/*exec('hostname -I', function(error, stdout, stderr) {
    console.log(('Please make sure to copy the ip address into path.txt: ' + stdout).green);
    //console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log(('exec error: ' + error).red);
    }
});*/

exec('hostname', function (error, stdout, stderr) {
  console.log(('Please make sure to copy the DNS name into hostname.txt: ' + stdout).green);
  //console.log('stderr: ' + stderr);
  if (error !== null) {
    console.log(('exec error: ' + error).red);
  }
});




