# 1. Environment prepration

Find openssl.cnf, for example, if the path is /usr/local/openssl/ssl/openssl.cnf, then typ commands as:

~$ cp /usr/local/openssl/ssl/openssl.cnf ./
~$ mkdir ./demoCA  ./demoCA/newcerts   ./demoCA/private
~$ chmod 777./demoCA/private
~$ echo '01'>./demoCA/serial
~$ touch./demoCA/index.txt
~$ sudo vim openssl.cnf

# 2. Generate CA

Generate a Certificate Authority:
	~$ openssl req -new -x509 -keyout ca-key.pem -out ca-crt.pem -config openssl.cnf

* Insert a CA Password and remember it
* Specify a CA Common Name, like 'root.localhost' or 'ca.localhost'. This MUST be different from both server and client CN.


# 3 Generate Server certificate

Generate Server Key:
	~$ openssl genrsa -out server-key.pem 1024

Generate Server certificate signing request:
	~$ openssl req -new -key server-key.pem -out server-csr.pem -config openssl.cnf

* Specify server Common Name, run   **cat /etc/hosts**    to check valid DNS name, please DO NOT name as **'localhost'**.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in server-csr.pem -out server-crt.pem -cert ca-crt.pem -keyfile ca-key.pem -config openssl.cnf

# 4 Generate Client certificate

Generate Client Key:
	~$ openssl genrsa -out client1-key.pem 1024

Generate Client certificate signing request:
	~$ openssl req -new -key client1-key.pem -out client1-csr.pem -config openssl.cnf

* Specify client Common Name, like 'client.localhost'. Server should not verify this, since it should not do reverse-dns lookup.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in client1-csr.pem -out client1-crt.pem -cert ca-crt.pem -keyfile ca-key.pem -config openssl.cnf

# 5 Generate CRL

Enviroment prepartion
	~$ cp ca-key.pem ./demoCA/private/cakey.pem
	~$ cp ca-crt.pem ./demoCA/cacert.pem
	~$ echo '00' >./demoCA/crlnumber

Revoke Server Certificate
	~$ openssl ca -revoke server-crt.pem -config openssl.cnf

Generate Server CRL
	~$ openssl ca -gencrl  -out server.crl  -config openssl.cnf

Revoke Client Certificate
	~$ openssl ca -revoke client1-crt.pem -config openssl.cnf

Generate Client CRL
	~$ openssl ca -gencrl  -out client1.crl  -config openssl.cnf

