# Secure Talk

The secure talk project is aimed to provide secure, TLS-encrypted bidirectional websocket communication between server and clients

* It supports TLS protocol for authentication & encryption on both server and clients.
* It offers wrapper classes to wrap complex low-level communication setup & implementation, which achieves a decouple between application and low-level infrastructure.
* There are three basic classes: secure_server, secure_client & client_cli.

## Secure_server

``Secure_server`` is an http/https server with websocket protocol. It supports both TCP/IP socket & Unix domain socket. Te Unix domain socket will use http server, while the TCP/IP socket will use https server. The reason is straight forward that the access to unix domain socket can be controlled by file permission by Linux file system and the access TCP/IP socket can only be controlled by application layer protocol such as TLS authentication.</br>

The server uses jwt token for reconnection. Once startup, the server will randomly generate a key, and the use it to sign the jwt. The reconnection feature is enabled as follow



| client  | server |
| -----  | ----  |
|  do nothing      | server generate HMAC key |
| connects | server recognizes the client and assigns the HMAC assigned jwt to client |
| disconnect | do nothing |
| reconnect with jwt | server verify the jwt with its key, if correct, the server maintains the login status |

The server encapsulates unix socket server, https server & websocket servers for admin and data respectively.

### Secure_server API
1. new SecureServer(options)
- `options`:
    - `host` {String}: host for https server
    - `port` {String}: port for https server
    - `socket` {String}: path for unix socket server
    - `key` {Buffer}: privateKey
    - `cert` {Buffer}: server certificate
    - `requestCert` {Boolean}: whether test client certificate
    - `ca` {Buffer}: CA certificate
2. SecureServer.adminUse(app)
- `app(ws, result, adminCtx)` {Function} the application to process admin websocket connection
    - `ws` {Websocket}: the websocket connection
    - `result` {String | Buffer}: the result parsed by the websocket parser.
    - `adminCtx` {Object}: the admin resources for manage connections.
3. SecureServer.dataUse(app)
- `app(ws, result, adminCtx)` {Function} the application to process data websocket connection
    - `ws` {Websocket}: the websocket connection
    - `result` {String | Buffer}: the result parsed by the websocket parser.
    - `adminCtx` {Object}: the admin resources for manage connections.
4. SecureServer.unixUse(app)
- `app(ws, result, adminCtx)` {Function} the application to process unix connection
    - `ws` {Websocket}: the websocket connection
    - `result` {String | Buffer}: the result parsed by the websocket parser.
    - `adminCtx` {Object}: the admin resources for manage connections.
5. SecureServer.start()
- start the server

6. SecureServer.close()
- close the server

## Secure_client

``Secure_client`` is a websocket client that supports jwt token to maintain connection status. Once boot up, the client will first use it's token to query the server whether it is still registered in the system. The server will assign a new token to the client if the predefined token is empty or invalid. After that, the client will setup a websocket connection with the server.</br>

### Secure_client API
1. new SecuredWebsocketClient(options)
- `options`
    - `socketPath` {String}: unix socket address
    - `host` {String}: host for https
    - `port` {String}: port fir https
    - `key`: {Buffer}: privateKey
    - `cert` {Buffer}: client certificate
    - `checkServerIdentity` {Function}: server identity check
    - `ca` {{Buffer}}: CA certificate
2. SecuredWebsocketClient.send(data,options)
- `data` {Any}: The data to send
- `options`
    - `compress` {Boolean} Specifies whether `data` should be compressed or not.
    Defaults to `true` when permessage-deflate is enabled. Options can be passed down.
    - `binary` {Boolean} Specifies whether `data` should be sent as a binary or
    not. Default is autodetected.
    - `mask` {Boolean} Specifies whether `data` should be masked or not. Defaults
    to `true` when `websocket` is not a server client.
    - `fin` {Boolean} Specifies whether `data` is the last fragment of a message
    or not. Defaults to `true`.
    - `callback` {Function} An optional callback which is invoked when `data` is written out.
3. SecuredWebsocketClient.close()
- close the websocket connection

### Server implements tricks
1. The context that application will use

| name | usage |
| ---- | ---- |
| client2pipe | a map `key` client_id, `value` a set of spawned pipes |
| pipe2socket | a map `key` pipe_id, `value` pipe socket |
| wsConns | a map `key` client_id, `value` websocket conn |
| adminWS | the admin websocket server |
| dataWS | the data websocket server |
| pipe2pid | a map `key` pipe_id, `value` father id & process handler |
| pipe2json | a map `key` pipe_id, `value` pipe json files |

## Client_cli
``Client_cli`` is an interactive interface between user input and web connection.</br>

The CLI provides a framework to dispatch user input to different processing functions.

### Client_cli API
1. new clientCLI(options)
- `options`
    - `socketPath` {String}: unix socket address
    - `host` {String}: host for https
    - `port` {String}: port for https
    - `key` {Buffer}: privateKey
    - `cert` {Buffer}: client certificate
    - `checkServerIdentity` {Function}: server identity check
    - `ca` {{Buffer}}: CA certificate
    - `completer`: completer function for user input
1. clientCLI.setExec(cmdDispatcher)
- `cmdDispatcher` {Function}: dispatches user input to different worker function
    - `args` {String}: user input
    - `return` {Function}: the worker function
2. clientCLI.close()
- close CLI interface & web connection

## Protocol exposed between websocket C/S

```javascript
{
    "headers": {
        "method": 'text',
        "pipe_id": "1"
    },
    "payload": "",
    "checkSum": ""
}
```

## Protocol in ipc
There are two types of protocol, one is the raw byte protocol, another is the json format protocol
### JSON format
```javascript
{
    "type": 0
    //0 for text, 1 for pipe_id, 2 for config 3 for launch 4 for property 5 for destroy
    "payload": "111222"
    // the message to be transferred
}
```
### Raw bytes format
| offset | meaning |
| ----- | ----- |
| 0 ~ 3 byte | total package length |
| 4 ~ 7 byte | message type |
| 8 byte ~ 8 + length byte | payload |

Example for sending ``hello world``

| offset | bits |
| ----- | ----- |
| 0 ~ 3 byte | 0x00 00 00 13 |
| 4 ~ 7 byte | 0x00 00 00 02 |
| 8 byte ~ 8 + length byte | 0x68 65 6C 6C 6F 20 77 6F 72 6C 64  |