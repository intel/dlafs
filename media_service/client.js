const WebSocket = require('ws');
//let count = 0;
//const ws = new WebSocket("ws://localhost:8124/sendData", {
//      protocolVersion: 8,
//      origin: 'https://127.0.0.1:4321',
 //     rejectUnauthorized: false
 //   });
const ws = new WebSocket("wss://localhost:8124/sendData", {
    rejectUnauthorized: false
  });

const fs = require('fs');

ws.on('open', function () {
    console.log(`[CLIENT3] open()`);
});

ws.on('message', function (data) {
    console.log("[CLIENT3] Received:",data);
    var buff =  new Buffer(data);
         var fd = fs.openSync('re_mini.jpg', 'w');
         fs.write(fd, buff, 0, buff.length, 0, function(err, written) {
    console.log('err', err);
    console.log('written', written);
});

});

ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 
 
