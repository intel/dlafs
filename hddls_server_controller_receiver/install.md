
#1. install nodejs packages
	npm config set proxy <proxy>
	cd hddls_server_controller_receiver 
	npm install

#2. setup certificates
Refer to certificate_create_explanation.md
	Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'controller/client_cert'
	Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'receiver/client_cert'
	Copy "ca-crt.pem server-crt.pem server-key.pem" into 'server/server_cert'
	Copy 'client1.crl' into 'server_cert'
	Copy 'server.crl' into 'client_cert'

    Change all pem files mode to be 400
       chmod 400 client_cert/*.pem
       chmod 400 server_cert/*.pem
    Change client_cert and server_cert mode to be 700
       chmod 700 client_cert
       chmod 700 server_cert

#3. run hddls-server
Please run below commands:
	cd server
	export HDDLS_CVDL_MODEL_PATH=`pwd`/models
	node hddls-server.js
Note: make sure models directory mode is 700
      make sure models/xxx/*.bin mode is 600
	  make sure server_cert/*.pem mode is 400


#4. run hddls receiver client
Open a new terminal, and run below command:
	cd receiver
	node result_receiver.js

Note: make sure models directory mode is 700
      make sure models/xxx/*.bin mode is 600
      make sure server_cert/*.pem mode is 400


#5. run hddls controller client
Open a new terminal, and run below command:
	cd controller
	node controller_cli.js

Note: make sure server_cert/*.pem mode is 400

