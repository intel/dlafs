#  Some Q&A about CA and crl
### Can client and server use different CA?

* Websockets support different CA for client and server. Howerver, if CA is self-signed, TLS wouldn't connect successfully. Here are two ways to use different CA:

1. use different <span style="color:yellow"> public CA </span> for server and client

2. if suing <span style="color:yellow"> self-signed CA </span>, set <span style="color:green"> rejectUnauthorized: true </span> to  <span style="color:green"> rejectUnauthorized:false </span> when set websocket

### How to use different CA to certificate client and server?

* Here are two ways:

1. if using public CA, just generate server and client certificate separately

2. if using self-signed CA, all steps below should be handled separately in server and client side.

### How to verify crl?

* Here are two ways to verify crl:

1. if using public CA, just follow steps from <span style="color:yellow">enviroment prepration </span> to <span style="color:yellow">generate crl</span>. 

2. if using self-signed CA, please make sure that server and client are using <span style="color:yellow"> the same CA </span>. Different self-signed CA would cause problems.

# 1. Environment prepration

find
<span style="color:yellow"> openssl.cnf </span>, for example, if the path is <span style="color:green">/usr/local/openssl/ssl/openssl.cnf</span>, then typ commands as:

```console
root@root:~$ cp   /usr/local/openssl/ssl/openssl.cnf ./
root@root:~$ mkdir ./demoCA  ./demoCA/newcerts   ./demoCA/private
root@root:~$ chmod 777./demoCA/private
root@root:~$ echo '01'>./demoCA/serial
root@root:~$ touch./demoCA/index.txt
```

# 2. Generate CA

<span style="color:orange">Generate a Certificate Authority:</span>
```console
root@root:~$ openssl req -new -x509 -keyout ca-key.pem -out ca-crt.pem -config openssl.cnf
```
* Insert a CA Password and remember it
* Specify a CA Common Name, like 'root.localhost' or 'ca.localhost'. This MUST be different from both server and client CN.


# 3 Generate Server certificate

<span style="color:orange">Generate Server Key:</span>
```console
root@root:~$ openssl genrsa -out server-key.pem 1024
```
<span style="color:orange">Generate Server certificate signing request:</span>
```console
root@root:~$ openssl req -new -key server-key.pem -out server-csr.pem -config openssl.cnf
```
* <span style="color:red"> Specify server Common Name, run   **cat /etc/hosts**    to check valid DNS name, please DO NOT name as **'localhost'**.
For this example, do not insert the challenge password.</span>

<span style="color:orange">Sign certificate using the CA:</span>
```console
root@root:~$ openssl ca -in server-csr.pem -out server-crt.pem -cert ca-crt.pem -keyfile ca-key.pem -config openssl.cnf
```

# 4 Generate Client certificate
<span style="color:orange">Generate Client Key:</span>
```console
root@root:~$ openssl genrsa -out client1-key.pem 1024
```
<span style="color:orange">Generate Client certificate signing request:</span>
```console
root@root:~$ openssl req -new -key client1-key.pem -out client1-csr.pem -config openssl.cnf
```
* Specify client Common Name, like 'client.localhost'. Server should not verify this, since it should not do reverse-dns lookup.
*    For this example, do not insert the challenge password.

<span style="color:orange">Sign certificate using the CA:</span> 
```console
root@root:~$ openssl ca -in client1-csr.pem -out client1-crt.pem -cert ca-crt.pem -keyfile ca-key.pem -config openssl.cnf
```



# 5 Generate CRL
<span style="color:orange">Enviroment prepartion</span>
```console
 root@root:~$ cp ca-key.pem ./demoCA/private/cakey.pem
 root@root:~$ cp ca-crt.pem ./demoCA/cacert.pem
 root@root:~$ echo '00' >./demoCA/crlnumber
```
<span style="color:orange">Revoke Server Certificate</span>
```console
 root@root:~$ openssl ca -revoke server-crt.pem -config openssl.cnf
 ```
 <span style="color:orange">Generate Server CRL</span>
 ```console
root@root:~$ openssl ca -gencrl  -out server.crl  -config openssl.cnf
 ```
<span style="color:orange">Revoke Client Certificate</span>
 ```console
root@root:~$ openssl ca -revoke client1-crt.pem -config openssl.cnf
```
 <span style="color:orange">Generate Client CRL</span>
  ```console
 root@root:~$ openssl ca -gencrl  -out client1.crl  -config openssl.cnf
 ```
