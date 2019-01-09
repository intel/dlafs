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
'use strict';
const wsSender = require('./ws_sender.js');
const { spawn } = require('child_process');
const path = require('../lib/path_parser')
const fileHelper = require('../lib/file_helper');
const fs = require('fs');
var pipe_base = 0;
exports.router = function route(obj) {
    return function(message) {
        var method = message.headers.method || 'unknown';
        console.log('\x1b[34mDispatcher %s\x1b[0m', method);
        if(!obj[method]) {
            return null;
        }
        var fn = obj[method];
        return fn;
    }
}

exports.createHandler = function createHandler(ws, message, adminCtx) {
    var create_json = fileHelper.safelyJSONParser(message.payload);
    if(create_json === null) {
        wsSender.sendMessage(ws, 'json format error', 400);
        return;
    }
    for (let i = 0; i < create_json.command_create.pipe_num; i++) {
        let pipe_id = pipe_base ++;
        let gst_cmd = `hddlspipes -i  ${pipe_id} -l ${create_json.command_create.loop_times} -u ${adminCtx.socketURL}`;
        console.log("prepare to launch %s", gst_cmd);
        let pipes;
        if(adminCtx.client2pipe.has(ws.id)) {
            pipes = adminCtx.client2pipe.get(ws.id);
        } else {
            pipes =  new Set();
            adminCtx.client2pipe.set(ws.id, pipes);
        }
        let child = spawn(gst_cmd, {
            shell: true
        });

        // child.stderr.on('data', function (data) {
        //     console.error("STDERR:", data.toString());
        // });

        child.stdout.on('data', function (data) {
           console.log("STDOUT:", data.toString());
        });

        child.on('exit', function (exitCode) {
            let pipe2delete = pipe_id;
            console.log("Child exited with code: " + exitCode);
            console.log("delete pipe %s", pipe2delete);
            pipes.delete(pipe2delete);
            adminCtx.pipe2pid.delete(pipe2delete);
            adminCtx.pipe2json.delete(pipe2delete);
            //update controller when pipeline exit
            if (ws.readyState === ws.OPEN) {
                ws.send(JSON.stringify({headers: {method: 'pipe_delete'}, payload: [pipe2delete], code: 200}));
            }
        });

        pipes.add(pipe_id);
        adminCtx.pipe2pid.set(pipe_id, {cid: ws.id, child: child});
        updatePipeJSON(adminCtx.pipe2json, create_json, pipe_id, 'create');
        console.log(adminCtx.pipe2json);
        console.log('create pipe %s', pipe_id);
        wsSender.sendMessage(ws, `pipe_create ${pipe_id}`);
        ws.send(JSON.stringify({headers: {method: 'pipe_id'}, payload: Array.from(pipes), code: 200}));
    }
}

exports.propertyHandler =  function propertyHandler(ws, message, adminCtx) {
    var property_json = fileHelper.safelyJSONParser(message.payload);
    var pipe_id = message.headers.pipe_id;
    console.log(message.headers);
    if(property_json === null) {
        wsSender.sendMessage(ws, 'json format error', 400);
        return;
    }
    console.log(`Setting property`);
    if(adminCtx.pipe2socket.has(pipe_id)) {
        var socket = adminCtx.pipe2socket.get(pipe_id);
        updatePipeJSON(adminCtx.pipe2json, property_json, pipe_id, 'property');
        //socket.send(message.payload, 3);
        property_json.command_set_property.pipe_id = pipe_id;
        socket.send(JSON.stringify(property_json));
    } else {
        console.log(`pipe_id ${pipe_id} not exists`)
    }

}

exports.destroyHandler = function destroyHandler(ws, message, adminCtx) {
    var destroy_json = fileHelper.safelyJSONParser(message.payload);
    if(destroy_json === null) {
        wsSender.sendMessage(ws, 'json format error', 400);
        return;
    } else if (!adminCtx.client2pipe.has(ws.id)) {
        wsSender.sendMessage(ws, `client ${ws.id} has no pipes yet`, 400);
        return;
    }
    var pipes = adminCtx.client2pipe.get(ws.id);
    var pipe_id = parseInt(message.headers.pipe_id);
    if(pipes.has(pipe_id)){
        var socket = adminCtx.pipe2socket.get(pipe_id);
        if(!!socket) {
            console.log('delete pipe %s', pipe_id);
            //socket.send(message.payload, 3);
	    destroy_json.command_destroy.client_id = ws.id;
            destroy_json.command_destroy.pipe_id = message.headers.pipe_id;
            socket.send(JSON.stringify(destroy_json));
        } else {
            console.log("Cannot find pipe socket %s",pipe_id);
        }

        adminCtx.pipe2socket.delete(pipe_id);
        adminCtx.pipe2pid.delete(pipe_id);
        adminCtx.pipe2json.delete(pipe_id);
        pipes.delete(pipe_id);
    } else {
        wsSender.sendMessage(ws, `pipe ${pipe_id} not exists`, 400);
    }
}

function updatePipeJSON(pipe2json, obj, pipe_id, type) {
    var pipeJSON = pipe2json.get(pipe_id) || {};
    pipeJSON[type] = obj;
    pipe2json.set(pipe_id, pipeJSON);
}

exports.mockCreateHandler =  function (ws, message, adminCtx) {
    var create_json = fileHelper.safelyJSONParser(message.payload);
    if(create_json === null) {
        wsSender.sendMessage(ws, 'json format error', 400);
        return;
    }
    var pipe_id = pipe_base ++;
    for (let i = 0; i < create_json.command_create.pipe_num; i++) {
      	var gst_cmd = './socket_client  /tmp/unix.sock ' + pipe_id;
	//var gst_cmd = 'hddlspipes -i  ' + pipe_id + ' -l 1 -u ' + 'ws://127.0.0.1:10086';
	console.log('cmd %s', gst_cmd);
        var pipes;
        if(adminCtx.client2pipe.has(ws.id)) {
          pipes = adminCtx.client2pipe.get(ws.id);
        } else {
            pipes =  new Set();
            adminCtx.client2pipe.set(ws.id, pipes);
        }
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
        pipes.delete(pipe_id);
        adminCtx.pipe2pid.delete(pipe_id);
        ws.send(JSON.stringify({headers: {method: 'pipe_id'}, payload: Array.from(pipes), code: 200}));
      });

      pipes.add(pipe_id);
      adminCtx.pipe2pid.set(pipe_id, {cid: ws.id, child: child});
      updatePipeJSON(adminCtx.pipe2json, create_json, pipe_id, 'create');
      console.log('create pipe %s', pipe_id);
      wsSender.sendMessage(ws, `pipe_create ${pipe_id}`);
      ws.send(JSON.stringify({headers: {method: 'pipe_id'}, payload: Array.from(pipes), code: 200}));
    }

}


exports.updateModel = function (ws, model, adminCtx){
    if(!model.headers.hasOwnProperty('path')) {
        throw new Error('x1b[31mNo Path in Headers\x1b[0m');
    }
    var filePath = model.headers.path;
    var dir = path.dirname(path.dirname(filePath));
    saveBuffer(filePath, model.payload);
    var modelMeta = JSON.stringify(fileHelper.updateCheckSum(filePath, model.checkSum));
    fs.writeFileSync(path.join(dir, 'model_info.json'), modelMeta);
	console.log('sendMeata');
    wsSender.sendProtocol(ws,{method: 'checkSum'}, modelMeta);
  }
  function saveBuffer(filePath, buffer) {
    var folderName = path.dirname(filePath);
    if(!fs.existsSync(folderName))
    {
      console.log('making dir %s', './' + folderName);
      fs.mkdirSync('./' + folderName, { recursive: true });
    }
    var stream = fs.createWriteStream(filePath);
    stream.on("open", ()=>{
      stream.write(buffer);
      stream.end();
    });
  }
