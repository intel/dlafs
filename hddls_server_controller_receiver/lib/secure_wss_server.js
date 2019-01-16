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
const WebSocket = require('ws');
const https = require('https');
const http = require('http');
const jwt = require('jsonwebtoken');
const EventEmitter = require('events');
const url = require('url');
const fs = require('fs');
const net = require('net');
const crypto = require('crypto');
const fileHelper = require('./file_helper');
const Transceiver = require('./transceiver');
const wsReceiver = require('./ws_receiver');
const wsSender = require('./ws_sender');
const constants = require("./constants");
var client_id = 0;
var secretKey = makeID();

function makeID() {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (var i = 0; i < 5; i++)
      text += possible.charAt(Math.floor(Math.random() * possible.length));
    return text;
}

function add(req,res) {
    res.end();
}

function genToken() {
    client_id = client_id + 1;
    var token = jwt.sign({client_id}, secretKey);
    console.log(token);
    return token;
}

function auth(req, res, secretKey) {
    if(!('token' in req.headers)) {
        res.write(genToken());
        res.end();
    } else {
        jwt.verify(req.headers['token'],secretKey,(err, decoded) => {
            if(!err){
                res.write(req.headers['token']);
                res.end();
            } else{
                res.write(genToken());
                res.end();
            }
        });
    }
}

function handleRequest(req,res) {
    if('/' == req.url || '/data' == req.url) {
        switch (req.method) {
            case 'GET':
                auth(req,res, secretKey);
            case 'POST':
                add(req,res);
        }
    }
}

class SecureServer extends EventEmitter {
    constructor(options) {
        super();
        this._options = Object.assign({
            socket: options.socket || process.platform === "win32" ? "\\\\.\\pipe\\ipc" : '/tmp/unix.sock'
        },options);
        this._httpsServer = null;
        this._adminWS = null;
        this._dataWS = null;
        this._ipcServer = null;
        this._client2pipe = new Map();
        this._adminApp = null;
        this._dataApp = null;
        this._unixApp = null;
        this._pipe2socket = new Map();
        this._pipe2pid = new Map();
        this._pipe2json = new Map();
        this._wsConns = new Map();
        this._dataCons = null;
    }

    initServer(options) {
        if(options.port == null && options.host == null) {
            throw new TypeError('No port specified');
        }
        //unix socket doesn't need port set.
        var server;
        if(options.port) {
            server = this._httpsServer = https.createServer(options);
            console.log("Listen on port %s", options.port);
        } else {
            server = this._httpsServer = http.createServer(options);
            console.log("Listen on host %s", options.host);
        }

        const adminWS = this._adminWS = new WebSocket.Server({
                noServer: true,
                verifyClient: verifyClient
        });

        const dataWS = this._dataWS = new WebSocket.Server({
            noServer: true,
            verifyClient: verifyClient
        });

        const ipcServer = this._ipcServer = getUnixSocketServer({socket: options.socket});
        const WSOptions = Object.assign({
            admin: adminWS,
            data: dataWS,
            dataPath: options.dataPath || '/data',
            adminPath: options.adminPath || '/'
        });
        server.on('request', handleRequest);
        server.on('upgrade', getUpgradeHandle(WSOptions));
        const adminCtx = {
            client2pipe: this._client2pipe,
            pipe2socket: this._pipe2socket,
            wsConns: this._wsConns,
            adminWS: adminWS,
            dataWS: dataWS,
            pipe2pid: this._pipe2pid,
            pipe2json: this._pipe2json,
            dataCons: this._dataCons,
            socketURL: this._options.socket
        };
        adminWS.on('connection', getConnHandler.call(options, this._adminApp, adminCtx));
        dataWS.on('connection', getDataConnHandler.call(options, this._dataApp, adminCtx));
        ipcServer.on('connection', getUnixConnHandler(this._unixApp, adminCtx, options.ipcProtocol || 'json'));
        server.listen(options.port ? options.port : options.host);
        ipcServer.listen({path: options.socket, readableAll: false, writableAll: false});
        fs.chmodSync(options.socket, 0o770);

    }

    start() {
        this.initServer(this._options);
    }

    close(cb) {
        console.log("server being closed");
        if (cb) this.once('close', cb);
        var cleanup = ['_dataWS', '_httpsServer', '_ipcServer', '_adminWS'];
	this._pipe2pid.forEach((value, key, map) => {
	    try {
		   if(!!value.child) {
			console.log(`kill pipe_id ${key} with pid ${value.child.pid}`);
			value.child.kill()
		   }
	    } catch (err) {
	    	console.log('Got Error %s', err.message);
	    }
	});
        for(const item of cleanup) {
            !! this[item] && this[item].close(()=>{console.log(`${item} close`)}) && (this[item] = null);
        }
    }

    adminUse(app) {
        this._adminApp = app;
    }

    dataUse(app) {
        this._dataApp = app;
    }

    unixUse(app) {
        this._unixApp = app;
    }
}

function verifyClient(info, done) {
    console.log('verify client');
    var pass = true;
    if (! ('token' in info.req.headers))
    {
        pass = false;
        console.log('no token');
    } else {
        jwt.verify(info.req.headers['token'],secretKey,(err, decoded) => {
            if(err){
                console.log('jwt error %s', err);
                pass = false;
            } else{
                pass = true;
                info.req.headers['token'] = decoded;
                console.log('decoded token id %s', decoded.client_id);
            }
        });
    }
    done(pass);
}

function addListeners(server, map) {
    for (const event of Object.keys(map)) server.on(event, map[event]);
    return function removeListeners() {
      for (const event of Object.keys(map)) {
        server.removeListener(event, map[event]);
      }
    };
}

function getConnHandler(app, adminCtx){
    var that = this;
    return function connection(ws, request) {
        ws.id = parseInt(request.headers['token'].client_id);
        adminCtx.wsConns.set(parseInt(request.headers['token'].client_id), ws);
        //console.log(adminCtx.wsConns);
        ws.on('message', function handle(message) {
            var result = wsReceiver.websocketParser(message);
            if(!!app && result != null) {
                try{
                    app(ws, result, adminCtx);
                } catch(err) {
                    console.log('Server Handle Got error %s', err.message);
                }
            }
        });
        ws.on('close', (code, reason)=> {
            console.log(`ws client ${ws.id} close reason ${code}/${reason}`);
        })
        if(adminCtx.client2pipe.has(ws.id)) {
            console.log('Client Reconnect id %s', ws.id);
            let pipes = adminCtx.client2pipe.get(ws.id);
            wsSender.sendProtocol(ws,{method: 'pipe_id'}, Array.from(pipes), 200);
        }
        fileHelper.uploadFile(
            [that.metaPath],
            ws,
            {method: 'checkSum'}
        );
    }
}

function getDataConnHandler(app, adminCtx){
    var that = this;
    return function connection(ws, request) {
        //ws.id = parseInt(request.headers['token'].client_id);
        ws.id = 1;
        adminCtx.dataCons = ws;
        ws.on('message', function handle(message) {
            var result = message;
            if(!!app) {
                try{
                    app(ws, result, adminCtx);
                } catch(err) {
                    console.log(err.message);
                }
            }
        });
        ws.on('close', (code, reason)=> {
            console.log(`ws client ${ws.id} close reason ${code}/${reason}`);
        })
    }
}

function getUpgradeHandle(options) {
    return function upgrade(request, socket, head) {
        const pathname = url.parse(request.url).pathname;
        if (pathname === options.adminPath) {
            options.admin.handleUpgrade(request, socket, head, function done(ws) {
                options.admin.emit('connection', ws, request);
          });
        } else if (pathname === options.dataPath) {
            options.data.handleUpgrade(request, socket, head, function done(ws) {
                options.data.emit('connection', ws, request);
          });
        } else {
          socket.destroy();
        }
      }
}

function getUnixSocketServer(options) {

    const socket = options.socket;
    var server = null;
    function createServer(){
        console.log('Creating server.');
        var server = net.createServer();
        return server;
    }
    console.log('Checking for leftover socket.');

    try{
        var stats = fs.statSync(socket);
        console.log('Removing leftover socket.');
        fs.unlinkSync(socket);
    } catch(err) {
    }
    server = createServer();
    return server;
}

function getUnixConnHandler(app, adminCtx, protocol="json")
{
    const pipe2socket = adminCtx.pipe2socket;
    return function (stream) {
        var transceiver = new Transceiver(stream, (data)=> {
            if(data.type == constants.msgType.ePipeID) {
                console.log('pipe connection acknowledged %s', data.payload);
                transceiver.id = parseInt(data.payload);
                pipe2socket.set(parseInt(data.payload), transceiver);
            }
            try{
                !!app && app(data, Object.assign(adminCtx, {transceiver: transceiver}));
            } catch (err) {
                console.log(err.message);
            }
        });
        stream.on('error', ()=> {
            this.destroy();
        })
        transceiver.init();
    }
};

module.exports = SecureServer;
