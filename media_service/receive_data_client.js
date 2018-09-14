const WebSocket = require('ws');
var ab2str = require('arraybuffer-to-string');
const fs = require('fs');

const ws = new WebSocket("wss://localhost:8123/binaryEchoWithSize", {
    rejectUnauthorized: false
  });

let count=0;

ws.on('open', function () {
    console.log(`[RECEIVE_DATA_CLIENT] open()`);
});


ws.on('message', function (data) {
    console.log("[RECEIVE_DATA_CLIENT] Received:",data);
    if(data.byteLength >1024){
       count++;
       var buff =  new Buffer(data);
        var image_name='image_'+count+'.jpg';
         var fd = fs.openSync(image_name, 'w');
         fs.write(fd, buff, 0, buff.length, 0, function(err, written) {
    console.log('err', err);
    console.log('written', written);
});
} else{
       var uint8 = new Uint8Array(data);
       fs.appendFile('output.txt', ab2str(uint8)+ "\n", function (err) {
  if (err) {
    console.log("append failed");
  } else {
    // done
    console.log("done");
  }
})
}
    

});

ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 
 
