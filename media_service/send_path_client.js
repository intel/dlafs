const WebSocket = require('ws');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});



const ws = new WebSocket("wss://localhost:8126/sendPath", {
    rejectUnauthorized: false
  });


ws.on('open', function () {
    console.log(`[SEND_PATH_CLIENT] open()`);
    var buf='/home/HDDL/data/video/1600x1200.264';
    ws.send(buf);
    console.log(`[SEND_PATH_CLIENT] path send!`);
});

ws.on('message',function(data){
	console.log(`right now we have ${data} pipes`);
	if(ws.readyState === WebSocket.OPEN){
    rl.question('how many pipes do you want to start? ', (answer) => {
    	if(typeof answer === 'numbers'){
    		console.log(`all right, we will start new ${answer} pipes`);
            ws.send(answer);
    	} else{
    		console.log('pleae type right format');
    		rl.close();
    		ws.close();
    	}

    
    });
  }  

});


ws.on('error', function () {
    console.log(`connect wrong!`);
    
}); 

ws.on('close', function () {
    console.log(`Right now I'll clear all pipes!`);
    
});
 

