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
'use strict'
const clientCLI = require('../lib/client_cli');
const fs = require('fs');
const fileHelper = require('../lib/file_helper');
const path = require('../lib/path_parser');
var modelCheck = {};
var pipe_ids = new Set();
var pipe2info = new Map();
var crl = null;
if(fs.existsSync('./client_cert/server.crl')) {
  crl = fs.readFileSync('./client_cert/server.crl')
}
// CLI help tips
const tips = [ ('help                             ' + 'show all commands')
           ,('c <create.json>                  ' + 'create pipelines')
           ,('p <property.json> <pipe_id>      ' + 'set pipelines property')
           ,('d <destroy.json>  <pipe_id>      ' + 'destroy pipelines')
           ,('pipe                             ' + 'display pipes\' info')
           ,('q                                ' + 'exit')
           ,('m <model_path>                   ' + 'upload custom model files')
           ,('show                             ' + 'show models in server side')
].join('\n');


// CLI command line completer
function completer(line) {
    let completions = 'help|c <create.json> |p <property.json> <pipe_id> |d <destroy.json> <pipe_id> |pipe |client |m <folder path> |q'.split('|')
    var hints = [];
    var args = line.trim().split(' ');
    var files;
    try {
      if(args[1] && line.includes('/'))
      {
        files = fs.readdirSync(path.dirname(args[1]));
      } else {
        files = fs.readdirSync('./');
      }
    }
    catch{
      console.log(" error path pls check")
      files = []
    }
    if(args.length === 1) {
      hints = completions.filter((c) => c.startsWith(line));
    } else if(args.length >1) {
      for(let file of files) {
        if(file.startsWith(path.basename(args[1]))) {
          let hint = args.slice();
          hint[1] = path.join(path.dirname(args[1]), file);
          hints.push(hint.join(' '));
      }
    }
  }
  return [hints && hints.length ? hints : completions, line];
}


var clientOptions = {
  host: '127.0.0.1',
  port: 8445,
  ca: fs.readFileSync('./client_cert/ca-crt.pem'),
  checkServerIdentity: () => undefined,
  key: fs.readFileSync('./client_cert/client1-key.pem'),
  cert: fs.readFileSync('./client_cert/client1-crt.pem'),
  completer: completer
};

if(crl != null) {
  clientOptions.crl = crl;
}

//Get CLI user input parser&&router
function setup(options) {

  return function cmdDispatcher(args) {
    var dispatcher;
    var help = options.tips;
    var cmd = args.trim().split(' ');
    var pipe_ids = options.pipe_ids;
    var fn = null;
    var dispatcher = {
      'help':  function(ws, rl) {
        rl.emit('hint',`\x1b[33m${help}\x1b[0m`);
      },
      'c': function(ws, rl) {
        if (cmd.length !== 2) {
          rl.emit('hint', `wrong cmd ${args} please check`);
          return;
        }
        var headers = {method: "create"};
        fileHelper.uploadFile([cmd[1]], ws, headers, ()=> rl.prompt());
      },
      'p': (ws, rl) => {
        if (cmd.length !== 3) {
          rl.emit('hint', `wrong cmd ${args} please check`);
          return;
        }
        var pipe_id = parseInt(cmd[2]);
        if(pipe_ids.has(pipe_id)) {
          var headers = {pipe_id: pipe_id, method: 'property'};
          fileHelper.uploadFile([cmd[1]], ws, headers, ()=> rl.prompt())
        } else{
          rl.emit('hint', `pipe_id ${pipe_id} not exist `);
        }
      },
      'd': (ws, rl) => {
        if (cmd.length !== 3) {
          rl.emit('hint', `wrong cmd ${args} please check`);
          return;
        }
        var pipe_id = parseInt(cmd[2]);
        if(pipe_ids.has(pipe_id)) {
          var headers = {pipe_id: parseInt(cmd[2]), method: 'destroy'};
          fileHelper.uploadFile([cmd[1]], ws, headers, ()=> rl.prompt());
        } else {
          rl.emit('hint', `pipe_id ${pipe_id} not exist `);
        }
      },
      'pipe': function(ws, rl) {
        console.log('\x1b[33mtotal pipes \x1b[0m');
        let pipeString = '';
        pipe_ids.forEach((value, index, array) => {
          pipeString += ' ' + value;
        });
        console.log(`\x1b[33m${pipeString}\x1b[0m`);
        console.log('\x1b[33mdetailed pipe info \x1b[0m');
        pipe2info.forEach((value, key, map) => {
          console.log(`\x1b[33mpipe_id:${key} \npipe_info: \n ${JSON.stringify(value, null, 2)}\x1b[0m`);
        });
        rl.prompt();
      },
      'q': function(ws,rl) {
	      rl.emit('hint', `Bye ${process.pid}`);
        process.exit(0);
      },
      'm' : function(ws, rl) {
        if (cmd.length !== 2) {
          rl.emit('hint', `wrong cmd ${args} please check`);
          return;
        }
        var rootPromise = fileHelper.scanDir(cmd[1], true);
        var childPromise = rootPromise.then((folders, reject) => {
          if(! folders) throw new Error('folder empty or not exists');
			    var actualFolders = folders.filter(folder => fs.lstatSync(folder).isDirectory());
		      if(actualFolders.length !== 0)
		      {
		  	    console.log("folders to upload");
		  	    console.log(actualFolders);
		      }
          return Promise.all(actualFolders.map(folder => fileHelper.scanDir(folder)
          .then(files => {
            if(!files) return;
          return Promise.all(files.map(file => fileHelper.cmpFile(file, modelCheck)))
          .then((files) => {var tmp = []; files.forEach(ele => !!ele && tmp.push(ele));return tmp})
        })))},(reason)=>{rl.emit('hint', 'empty ' + reason.message)})
        .then((value) => {
          if(!value) return;
          var merged = [].concat.apply([], value);
          merged = merged.filter(function(el) { return el; });
          if(merged.length !==0) {
            console.log(`models Server need:`);
            console.log(merged);
          }
          fileHelper.uploadFile(merged, ws, {method: 'model'}, ()=> rl.prompt())
        }, error => rl.emit('hint', 'Scan child dir err ' + error));
        childPromise.catch(reason=>rl.emit('hint', 'Scan root dir err ' + reason.message));
      },
      'show' : function(ws, rl) {
        console.log("Current models in server");
        console.log(modelCheck);
        rl.prompt();
      }
    }

    if(dispatcher[cmd[0]]) {
      fn = dispatcher[cmd[0]];
    } else {
      fn = (ws, rl) => {rl.emit('hint', `wrong cmd ${args} please check`)};
    }
    return fn;
    }
}

//The websocket incoming parser && router
function getInParser() {
return function incoming(ws, message, rl) {
  var method = message.headers.method || 'unknown';
    if(method === 'text') {
      console.log('message %s', message.payload);
    } else if(method === 'pipe_id') {
        console.log("pipe_id from server", message.payload);
        pipe_ids.clear();
        message.payload.forEach(elem=> pipe_ids.add(elem));
        rl.prompt();
    } else if(method === 'pipe_delete') {
        console.log('pipe delete %s', message.payload);
        message.payload.forEach(elem=> {
          pipe_ids.delete(elem);
          pipe2info.delete(elem);
        });
        rl.prompt();
    } else if(method === 'checkSum'){
      	method ==='checkSum' && (modelCheck = fileHelper.safelyJSONParser(message.payload.toString()));
    } else if(method === 'pipe_info') {
      if(message.headers.hasOwnProperty('pipe_id')) {
        let pipeID = message.headers.pipe_id;
        pipe2info.set(pipeID, message.payload);
      }
    } else if(method === 'pipe_message') {

      console.log(`\x1b[31m\nERROR from pipe: ${message.headers.pipe_id}\x1b[0m`);
      console.log(message.payload);
      rl.prompt();
    }
  };
}


const cli = new clientCLI(clientOptions);

process.on('exit', ()=> {cli.close()});

cli.setInParser(getInParser());
cli.setExec(setup({tips: tips, pipe_ids: pipe_ids}));
cli.start();
