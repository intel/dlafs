"use strict"; // http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
const WebSocketServer = require('ws').Server;
const http = require('http');
const fs = require('fs');
const https = require('https');
let buff;

const server = http.createServer();
const wss = new WebSocketServer({server: server, path: '/foo'});
wss.on('connection', function(ws) {
    console.log('/foo connected');
    ws.on('message', function(data, flags) {
        //if (flags.masked) { return; }
        console.log('>>> ' + data);
        if (data == 'goodbye') { console.log('<<< galaxy'); ws.send('galaxy'); }
        if (data == 'hello') { console.log('<<< world'); ws.send('world'); }
    });
    ws.on('close', function() {
      console.log('/foo Connection closed!');
    });
    ws.on('error', function(e) {
      console.log('/foo Connection error!');
    });
});


//const binServer = https.createServer({
  //cert: fs.readFileSync('server-cert.pem'),
  //key: fs.readFileSync('server-key.pem'),
  //strictSSL: false
//});


const binServer = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
  //strictSSL: false
});
const wssBinaryEchoWithSize = new WebSocketServer({server: binServer, path: '/binaryEchoWithSize'});
wssBinaryEchoWithSize.on('connection', function(ws) {
    console.log("/binaryEchoWithSize is connected");
    ws.on('message', function(data) {
         console.log(data);
         buff =  new Buffer(data);  
    });
    ws.on('close', function() {
        console.log("/binaryEchoWithSize Connection closed!");
    });
    ws.on('error', function(e) {
        console.log('/binaryEchoWithSize Connection error!');
    });
});


const senServer = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
  //strictSSL: false
});
const sendData = new WebSocketServer({server: senServer, path: '/sendData'});

sendData.on('connection', function(ws) {
    console.log("/sendData is connected");
    let buf = new Uint8Array(buff).buffer;
    //console.log(buf);
    ws.send(buf);
    ws.on('close', function() {
        console.log("/sendData Connection closed!");
    });
    ws.on('error', function(e) {
        console.log('/sendData Connection error!');
    });
});


server.listen(8126);
senServer.listen(8124);
console.log('Express server started on port %s', senServer.address().port);
binServer.listen(8123);
console.log('Listening on port 8126,8124 and 8123...');

