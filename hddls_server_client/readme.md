# 1. download and build uWebSocket

uwebsockets: [uwebsockets](https://github.com/uNetworking/uWebSockets)

download and make install


# 2. setup nodejs enviroment

 apt-get install nodejs-legacy npm (please install nodejs version after 8.0)

 npm config set proxy <proxy>

 npm install ws

 npm install -g n

 npm install colors path fs childprocess readline

# 3. setup certificate
*please generate all these certificates in one pc!!!*

Please Refer to certificate_create_explanation.md.

### After generated, please copy demoCA into hddl_server and hddl_client, copy all file start with 'ca' and 'client' into 'cert_client_8216_8124', and copy all file start with 'ca' and 'server' into 'cert_server_8216_8124'.

<span style="color:red">ATTENTION: please copy 'client1.crl' into 'cert_server_8216_8124', copy  'server.crl' into 'cert_client_8216_8124'!!!</span>


# 4. run websocket server and client:

### write hostname into hostname.txt
- when there are serveral servers, please write hostnames by line.
### chmod a+x hddls_server.js receiver_client.js controller_client.js
### ./hddls_server.js
### ./controller_client.js
### ./receiver_client.js
