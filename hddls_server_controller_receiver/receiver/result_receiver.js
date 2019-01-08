const fs = require('fs');
const fileHelper = require('../lib/file_helper');
const constants = require('../lib/constants');
const SecureWebsocket = require('../lib/secure_client');

var options = {
  host: '127.0.0.1',
  port: 8445,
  ca: fs.readFileSync('./cert_client_8126_8124/ca-crt.pem'),
  checkServerIdentity: () => undefined,
  key: fs.readFileSync('./cert_client_8126_8124/client1-key.pem'),
  cert: fs.readFileSync('./cert_client_8126_8124/client1-crt.pem'),
  route: 'data'
};

var pipe2file = new Map();
for (let i = 0; i < 100; i++) {
  pipe2file.set(i, 0);
}

ws = new SecureWebsocket(options);

function incoming(data) {
  var metaData = Buffer.from(data.payload);
  console.log("payload type %s", typeof data.payload);
  console.log("receive type %s payload %s", data.headers.type, Buffer.isBuffer(metaData));
  var pipe_id = data.headers.pipe_id;
  var con = 'pipe_' + pipe_id.toString();
  let path = './' + con;
  if (data.headers.type == constants.msgType.eMetaJPG) {
      console.log('save JPG');
      let temp = pipe2file.get(pipe_id);
      let image_name = 'image_' + temp + '.jpg';
      let path = './' + con + '/' + image_name;
      fileHelper.saveBuffer(path, metaData);
      temp++;
      pipe2file.set(pipe_id, temp);
  } else if(data.headers.type == constants.msgType.eMetaText) {
      console.log('save txt');
      let path = './' + con + '/output.txt';
      fs.appendFile(path, metaData, function (err) {
          if (err) {
              console.log("append failed: ", err);
          } else {
              console.log("done".green);
          }
      })
    }
}

ws.on('message', incoming);
