
#1. install nodejs packages
	npm config set proxy <proxy>
	npm install

#2. setup certificates
Refer to certificate_create_explanation.md
	Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'controller/cert_client_8216_8124'
	Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'receiver/cert_client_8216_8124'
	Copy "ca-crt.pem server-crt.pem server-key.pem" into 'server/cert_server_8216_8124'
	Copy 'client1.crl' into 'cert_server_8216_8124'
	Copy 'server.crl' into 'cert_client_8216_8124'

Note:
    Please generate all these certificates in one pc!!!


#3. run hddls-server
Please run below commands:
	cd server
	node hddls-server.js


#4. run hddls receiver client
Open a new terminal, and run below command:
	cd receiver
	node result_receiver.js

#5. run hddls controller client
Open a new terminal, and run below command:
	cd controller
	node controller_cli.js
