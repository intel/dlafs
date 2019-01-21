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
const EventEmitter = require('events');
const https = require('https');
const http = require('http');
const fs = require('fs');
const WebSocket = require('ws');
const wsReceiver = require('./ws_receiver');

class SecuredWebsocketClient extends EventEmitter {
    constructor(options) {
        super();
        this._ws = null;
        this._token = null;
        this._options = options;
        this._isUnix = false;
        if(options.socketPath)
        {
            this._address = 'ws+unix://'+options.socketPath;
            console.log(this._address);
            this._isUnix = true;
        } else {
            if(!! options.route) {
                this._address = "wss://" + options.host + ":"+ options.port + "/" + options.route;
	console.log(this._address);
            }
            else {
                this._address = "wss://" + options.host + ":"+ options.port;
		console.log(this._address);
            }
        }
        this.on('TokenReady', this.saveToken);
        this.on('TokenReady', this.initWebsocket);

        fs.exists('./token.txt', (exists) => {
            if(exists) {
                console.log("token exists");
                this._token = fs.readFileSync('./token.txt', {encoding:'utf8'});
            }
                this.getToken(options);
        });

    }

    getToken(options) {
        options['headers'] = Object.assign({
            'token' : this._token
        });

        const httpObj = this._isUnix ? http : https;
        try{
        httpObj.get(options, (res) => {
            const { statusCode } = res;
            const contentType = res.headers['content-type'];
            let error;
            if (statusCode !== 200) {
              error = new Error('Request Failed.\n' +
                                `Status Code: ${statusCode}`);
            }
            if (error) {
              console.error(error.message);
              // consume response data to free up memory
              res.resume();
              return;
            }
            res.setEncoding('utf8');
            let rawData = '';
            res.on('data', (chunk) => { rawData += chunk; });
            res.on('end', () => {
              try {
                this._token = rawData;
                this.emit('TokenReady', rawData);
              } catch (e) {
                console.error(e.message);
              }
            });
          }).on('error', (e) => {
            console.error(`Got error: ${e.message}` + " please check your URL");
            process.exit(0);
          });
        } catch (err) {
            console.log(`Http Get error ${err.message}`);
            process.exit(0);
        }

    };

    initWebsocket() {

        this._options['headers'] = Object.assign({
            'token' : this._token
        });
        try {
            const ws = new WebSocket(this._address, this._options);
            this._ws = ws;
            ws.on('error', this.emit.bind(this, 'error'));
            ws.on('message', (message)=>{var result = wsReceiver.websocketParser(message); this.emit('message', result)});
            ws.on('open', this.emit.bind(this, 'open'));
            ws.on('close', this.emit.bind(this, 'close'));
        } catch (error) {
            this.emit('error', error);
            this.emitClose();
        }
    }

    saveToken(token) {
        fs.writeFile('./token.txt', token, (err)=>{
            if(err) {
                console.log("save file %s", err);
            }
        });
    }

    send(data, options) {
        if(this._ws) {
            this._ws.send(data, options);
        }
    }

    close() {
        if(this._ws) {
            this._ws.close();
            this.emit('close');
        }

    }
}


module.exports = SecuredWebsocketClient;
