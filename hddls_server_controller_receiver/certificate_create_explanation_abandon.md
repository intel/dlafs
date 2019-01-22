# 1. Environment prepration

~$ mkdir demoCA
~$ touch demoCA/index.txt

# 2. Generate CA

Generate a Certificate Authority:
	~$ openssl req -new -x509 -keyout ca-key.pem -out ca-crt.pem

* Insert a CA Password and remember it
* Specify a CA Common Name, like 'root.localhost' or 'ca.localhost'.


# 3 Generate Server certificate

Generate Server Key:
	~$ openssl genrsa -out server-key.pem 4096

Generate Server certificate signing request:
	~$ openssl req -new -key server-key.pem -out server-csr.pem

* Specify server Common Name.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in server-csr.pem -out server-crt.pem -cert ca-crt.pem -keyfile ca-key.pem

# 4 Generate Client certificate

Generate Client Key:
	~$ openssl genrsa -out client1-key.pem 4096

Generate Client certificate signing request:
	~$ openssl req -new -key client1-key.pem -out client1-csr.pem -config openssl.cnf

* Specify client Common Name, like 'client.localhost'. Server should not verify this, since it should not do reverse-dns lookup.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in client1-csr.pem -out client1-crt.pem -cert ca-crt.pem -keyfile ca-key.pem

# 5 Generate CRL

Enviroment prepartion
    ~$ touch ./demoCA/crlnumber
    ~$ echo "01" > demoCA/crlnumber

Revoke Server Certificate
    ~$ openssl ca -keyfile ca-key.pem -cert ca-crt.pem -revoke server-crt.pem
Generate Server CRL
    ~$ openssl ca -gencrl -keyfile ca-key.pem -cert ca-crt.pem -out server.crl

Revoke Client Certificate
    ~$ openssl ca -keyfile ca-key.pem -cert ca-crt.pem -revoke client1-crt.pem
Generate Client CRL
	~$ openssl ca -gencrl -keyfile ca-key.pem -cert ca-crt.pem -out client1.crl

# 6. Some Q&A about CA and CRL
  1) Can client and server use different CA?
     Websockets support different CA for client and server. Howerver, if CA is self-signed, TLS wouldn't connect successfully. Here are two ways to use different CA:
        a). Use different public CA for server and client
        b). Use self-signed CA, should set rejectUnauthorized:true to rejectUnauthorized:false when set websocket
  2) How to use different CA to certificate client and server?
     Here are two ways:
        a). Use public CA, just generate server and client certificate separately
        b). Use self-signed CA, all steps below should be handled separately in server and client side.
 3) How to verify CRL?
    Here are two ways to verify crl:
        a). If using public CA, just above process 1~5 
        b). If using self-signed CA, please make sure that server and client are using the same CA. Different self-signed CA would cause problems.

