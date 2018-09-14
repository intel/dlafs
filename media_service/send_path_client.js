const WebSocket = require('ws');

const ws = new WebSocket("wss://localhost:8126/sendPath", {
    rejectUnauthorized: false
  });


ws.on('open', function () {
    console.log(`[SEND_PATH_CLIENT] open()`);
    var buf='/home/HDDL/data/video/1600x1200.264';
    ws.send(buf);
});


ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 
 
