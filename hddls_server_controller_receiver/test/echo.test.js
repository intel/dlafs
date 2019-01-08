'use strict'

const fs = require('fs');
const SecureWebsocket = require('../lib/secure_client');
const SecureServer = require('../lib/secure_server');
const assert = require('assert');

var clientOptions = {
    host: '127.0.0.1',
    port: 10086,
    ca: fs.readFileSync('./client_cert/cert.pem'),
    checkServerIdentity: () => undefined,
    key: fs.readFileSync('./client_cert/client-key.pem'),
    cert: fs.readFileSync('./client_cert/client-cert.pem')
};

var privateKey  = fs.readFileSync('./server_cert/key.pem', 'utf8');
var certificate = fs.readFileSync('./server_cert/cert.pem', 'utf8');
var client_cert = fs.readFileSync('./server_cert/client-cert.pem', 'utf8');
var serverOptions = {port: 10086, key: privateKey, cert: certificate, requestCert: true, ca: [client_cert]};

function incoming(ws, message) {
    console.log('received: %s from client_id %s', message, ws.id);
    ws.send('Message from Server ' + message);
};

describe('client connected', () => {
    it('server inform client connection message', function(done) {
        const server = new SecureServer(serverOptions);
        server.use(incoming);
        server.start();
        const client = new SecureWebsocket(clientOptions);
        client.on('message', function incoming(data) {
            assert.strictEqual(data, 'WSServer Connected');
            server.close(done);
        });
        client.send('hello');
    });
})

