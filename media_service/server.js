"use strict"; // http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
const WebSocketServer = require('ws').Server;
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const https = require('https');
var spawn = require('child_process').spawn;


const path_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem')
});

const path_wss = new WebSocketServer({server: path_server, path: '/sendPath'});
path_wss.on('connection', function(ws) {

    console.log('/sendPath connected');
    ws.on('message', function(path) {

        console.log('>>> ' + path);

        var gst_cmd='gst-launch-1.0 filesrc location=' + path + ' ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res res.src_pic ! mfxjpegenc ! wssink name=ws res.src_txt ! ws.';
	console.log('gst_cmd = ' + gst_cmd);

        var child = spawn(gst_cmd , {
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
    });
   ws.on('close', function() {
        console.log('/sendPath Connection closed!');
   });
  ws.on('error', function(e) {
        console.log('/sendPath Connection error!');
  });
});



const data_server = https.createServer({
  cert: fs.readFileSync('server-cert.pem'),
  key: fs.readFileSync('server-key.pem'),
  strictSSL: false
});

const data_wss = new WebSocketServer({server: data_server, path: '/binaryEchoWithSize'});

// Broadcast to all.
data_wss.broadcast = function broadcast(data) {
  data_wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};

data_wss.on('connection', function connection(ws) {
  ws.on('message', function incoming(data) {
    // Broadcast to everyone else.
     console.log(data);
    /*wssBinaryEchoWithSize.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }*/
      data_wss.broadcast(data);
    });
  });



path_server.listen(8126);
data_server.listen(8123);
console.log('Listening on port 8126 and 8123...');
