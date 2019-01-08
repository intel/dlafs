const clientCLI = require('../lib/client_cli');

const fs = require('fs');
const SecureWebsocket = require('../lib/secure_client');
const SecureServer = require('../lib/secure_server');
const assert = require('assert');

var clientOptions = {
    host: '127.0.0.1',
    port: 8445,
    ca: fs.readFileSync('../client_cert/cert.pem'),
    checkServerIdentity: () => undefined,
    key: fs.readFileSync('../client_cert/client-key.pem'),
    cert: fs.readFileSync('../client_cert/client-cert.pem')
};

const cli = new clientCLI(clientOptions);


function myExec (line, ws) {
    ws.send(line+this.tail);
}

cli.setExec(myExec.bind({tail:" customFunctionTails"}));
cli.start();