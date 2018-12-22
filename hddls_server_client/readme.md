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

## Prepare certificates

    -Generate a Certificate Authority:

      openssl req -new -x509 -days 9999 -keyout ca-key.pem -out ca-crt.pem

    -Insert a CA Password and remember it
    -Specify a CA Common Name, like 'root.localhost' or 'ca.localhost'. This MUST be different from both server and client CN.

## Server certificate

    -Generate Server Key:

       openssl genrsa -out server-key.pem 4096

    -Generate Server certificate signing request:

       openssl req -new -key server-key.pem -out server-csr.pem

    -Specify server Common Name, run    cat /etc/hosts    to check valid DNS name, please DO NOT name as'localhost'.
    -For this example, do not insert the challenge password.

## Sign certificate using the CA:

       openssl x509 -req -days 9999 -in server-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out server-crt.pem

    -insert CA Password

## Verify server certificate:

       openssl verify -CAfile ca-crt.pem server-crt.pem

## Client certificate

    -Generate Client Key:

       openssl genrsa -out client1-key.pem 4096

    -Generate Client certificate signing request:

       openssl req -new -key client1-key.pem -out client1-csr.pem

    -Specify client Common Name, like 'client.localhost'. Server should not verify this, since it should not do reverse-dns lookup.
    -For this example, do not insert the challenge password.

## Sign certificate using the CA:

       openssl x509 -req -days 9999 -in client1-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out client1-crt.pem

    -insert CA Password

## Verify client certificate:

      openssl verify -CAfile ca-crt.pem client1-crt.pem

### After generated, please copy all file start with 'ca' and 'client' into 'cert_client_8216_8124', and copy all file start with 'ca' and 'server' into 'cert_server_8216_8124'.


# 4. run websocket server and client:

### write hostname into hostname.txt
- when there are serveral servers, please write hostnames by line.
### chmod a+x hddls_server.js receiver_client.js controller_client.js
### ./hddls_server.js
### ./controller_client.js
### ./receiver_client.js
