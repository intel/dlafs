
History of how to generate certificate/key:

---------------------------------------------------------------------------------------------------
Version 1.1
---------------------------------------------------------------------------------------------------
1. Generate root CA's key and cert

   a) Generate CA's private key, you need to prepare one password to protect this private key
   # openssl genrsa -aes256 -out ca-key.pem 4096

   b) Generate cert signing request file
   # openssl req -new -key ca-key.pem -out ca.csr

   c) Generate CA's self-signed cert, you need to input private key's password you prepared in step a)
   # openssl x509 -req -days 9999 -signkey ca-key.pem -in ca.csr -out ca-crt.pem

2. Generate Server's private key and cert

   a) Generate server's private key
   # openssl genrsa -out server-key.pem 4096

   b) Generate server's cert signing request file(Do not insert the challenge password)
   # openssl req -new -key server-key.pem -out server-csr.pem

     note: When OpenSSL prompts you for the Common Name for each certificate, use different names.

   c) Generate server's cert
   # openssl x509 -req -days 9999 -in server-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out server-crt.pem

   d) Verify server's cert
   # openssl verify -CAfile ca-crt.pem server-crt.pem

3. Generate Client's private key and cert

   a) Generate client's private key
   # openssl genrsa -out client1-key.pem 4096

   b) Generate client's cert signing request file(Do not insert the challenge password)
   # openssl req -new -key client1-key.pem -out client1-csr.pem

   c) Generate client's cert
   # openssl x509 -req -days 9999 -in client1-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out client1-crt.pem

   d) Verify client's cert
   # openssl verify -CAfile ca-crt.pem client1-crt.pem

4. Generate CRL ( Optional )

   a) # mkdir demoCA;touch demoCA/index.txt

   b) # touch ./demoCA/crlnumber;echo "01" > demoCA/crlnumber

   c) Revoke client cert if needed
   # openssl ca -keyfile ca-key.pem -cert ca-crt.pem -revoke client1-crt.pem

   d) Revoke server cert if needed
   # openssl ca -keyfile ca-key.pem -cert ca-crt.pem -revoke server-crt.pem

   e) Generate crl
   # openssl ca -gencrl -keyfile ca-key.pem -cert ca-crt.pem -out test.crl

   f) View the crl
   # openssl crl -noout -text -in test.crl

   g) rename test.crl
   # cp test.crl server.crl
   # cp test.crl client1.crl

5. Q&A about CA and CRL

  1) Q: Can clients and server use different CA?
     A: Yes.

  2) How to use different CA for client and server?
     A: First, you need to generate different root CA for clients and server, then sign client's cert with
        client's root CA and sign server's cert with server's CA.


 ---------------------------------------------------------------------------------------------------
 Version 1.0
 ---------------------------------------------------------------------------------------------------

1. Environment prepration

~$ mkdir demoCA
~$ touch demoCA/index.txt

2. Generate CA

Generate a Certificate Authority:
	~$ openssl req -new -x509 -keyout ca-key.pem -out ca-crt.pem

* Insert a CA Password and remember it
* Specify a CA Common Name, like 'root.localhost' or 'ca.localhost'.


3 Generate Server certificate

Generate Server Key:
	~$ openssl genrsa -out server-key.pem 4096

Generate Server certificate signing request:
	~$ openssl req -new -key server-key.pem -out server-csr.pem

* Specify server Common Name.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in server-csr.pem -out server-crt.pem -cert ca-crt.pem -keyfile ca-key.pem

4 Generate Client certificate

Generate Client Key:
	~$ openssl genrsa -out client1-key.pem 4096

Generate Client certificate signing request:
	~$ openssl req -new -key client1-key.pem -out client1-csr.pem -config openssl.cnf

* Specify client Common Name, like 'client.localhost'. Server should not verify this, since it should not do reverse-dns lookup.
* For this example, do not insert the challenge password.

Sign certificate using the CA:
	~$ openssl ca -in client1-csr.pem -out client1-crt.pem -cert ca-crt.pem -keyfile ca-key.pem

5 Generate CRL

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

6. Some Q&A about CA and CRL
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

