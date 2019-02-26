
Generate certificate and key for server and clients

-----------------------------------------------------------------------------------------------------------
|                 |  CA Key/Cert                              |         Key/Cert                          |
-----------------------------------------------------------------------------------------------------------
|  HDDL-S Server  |  EC keys(Curve prime256v1/secp256r1)/X509 | EC keys(Curve prime256v1/secp256r1)/X509  |
-----------------------------------------------------------------------------------------------------------
|  HDDL-S Client  |  EC keys(Curve prime256v1/secp256r1)/X509 | EC keys(Curve prime256v1/secp256r1)/X509  |
-----------------------------------------------------------------------------------------------------------

TLS configuration recommendation
-----------------------------------------------------------------------------------------------------------
|    Cipher Suite Name                    |  Rank |         OpenSSL Name            |      Reference      |
-----------------------------------------------------------------------------------------------------------
| TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 |   1   |  ECDHE-ECDSA-AES128-GCM-SHA256  |      [RFC5289]      |
-----------------------------------------------------------------------------------------------------------
| TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256   |   2   |  ECDHE-RSA-AES128-GCM-SHA256    |      [RFC5289]      |
-----------------------------------------------------------------------------------------------------------
 Note:
   a).Prefer Curve for EC: NIST P-256(aka prime256v1 or secp256r1)
   b).Recommend 3072-bit RSA moduli and 216+1 (65537) for your RSA public exponent.
   c).On HDDL-S Client machine - Keys need confidentiality and integrity protection. Certs and CRLs require integrity protection.


1. Generate root CA's key and cert

   a) Generate CA's private key, you need to prepare one password to protect this private key
   # openssl ecparam -genkey -name prime256v1 -out ca-key.pem

   b) Generate CA's self-signed cert, you need to input private key's password you prepared in step a)
   # openssl req -x509 -new -SHA256 -nodes -key ca-key.pem -days 9999 -out ca-crt.pem

2. Generate Server's private key and cert

   a) Generate server's private key
   # openssl ecparam -genkey -name prime256v1 -out server-key.pem

   b) Generate server's cert signing request file(Do not insert the challenge password)
   # openssl req -new -SHA256 -key server-key.pem -nodes -out server-csr.pem
     note: When OpenSSL prompts you for the Common Name for each certificate, use different names.

   c) Generate server's cert
   # openssl x509 -req -SHA256 -days 9999 -in server-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out server-crt.pem

   d) Verify server's cert
   # openssl verify -CAfile ca-crt.pem server-crt.pem

3. Generate Client's private key and cert

   a) Generate client's private key
   #  openssl ecparam -genkey -name prime256v1 -out client1-key.pem

   b) Generate client's cert signing request file(Do not insert the challenge password)
   #  openssl req -new -SHA256 -key client1-key.pem -nodes -out client1-csr.pem

   c) Generate client's cert
   #  openssl x509 -req -SHA256 -days 9999 -in client1-csr.pem -CA ca-crt.pem -CAkey ca-key.pem -CAcreateserial -out client1-crt.pem

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
