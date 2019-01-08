'strict mode'

const fs = require('fs');
const SecureWebsocket = require('../lib/secure_client');

var options = {
	socketPath: '/tmp/server.sock'
};
//socketPath for unix socket
//host port for internet socket
ws = new SecureWebsocket(options);

var stdin = process.openStdin();
stdin.addListener("data", function(d) {
	if(!ws){
		return
	}
	if (d.toString().trim()) {
		ws.send(d.toString().trim());
    	console.log("you entered: [" +
        	d.toString().trim() + "]");
	}

  });
  ws.on('message', function incoming(data) {
    console.log(data);
	});

// var ws = require('ws');
// var client = new ws("ws+unix:///tmp/server.sock")
// client.on('open', ()=> {client.send('hello')});