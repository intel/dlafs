'strict mode'

var WebSocketServer = require("../lib/secure_server");
const fs = require('fs');

var serverOptions = {
    host: '/tmp/server.sock'
    //host: '127.0.0.1',
    //port: 8445
};


server = new WebSocketServer(serverOptions);

function incoming(ws, message) {
    console.log('received: %s from client_id %s', message, ws.id);
    ws.send('Message from Server ' + message);
};

server.use(incoming);

server.start();


process.on('SIGTERM', function () {
    console.log("I am exiting");
    server.close(function () {
        console.log("I am exiting");
        process.exit(0);
    });
  });

process.on('SIGINT', () => {

    server.close();
});