media service based on WebSocket

1. setup server
sudo apt-get install nodejs-legacy npm
npm config set proxy http://child-prc.intel.com:913
npm install ws@6.0.0
npm cache clean -f
n stable"
n use 10.10.0 server.js

2. run client
receive_data_client.js
send_path_client.js -s <stream_source> -l <loop_times>
