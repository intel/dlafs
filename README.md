--------------
HDDL-S project
-------------

The software architecture of HDDL-S reference design extends its scope to whole computer vision media application. It is a full pipeline integrating video decoder, computer vision processing (including image pre-processing, deep learning processing includes but is not limited to object detection and classification/recognition, object tracking etc.), post-processing (including image encoder, metadata encapsulation, etc.) and network transmission. So in HDDL-S, MSDK, OpenCL, OpenCV and network protocols will be involved as well as the entire HDDL-R software stack via OpenVINO.
HDDL-S software architecture is based on GStreamer, which encapsulates all vision components so that can reach the good flexibility to build diversified vision applications. It is easy to build up kinds of vision application once the basic filters are implement.  HDDL-S can support multiple media channels, so a HDDL-S module is made up by multiple hddl channels, and each channel can setup a GStreamer pipeline. One webserver is created in a HDDL-S module to manage its all hddl channels, and provides vision processing service for multiple controller clients in host, and send vision processing result to receiver client.


HDDL-S SW contains 2 parts:

	1). HDDL-S pipeline stack
	It is based on OpenVINO R5 and running in gstreamer framework to cover all CV/DL tasks.

	2). HDDL-S server/client
	It provides NodeJS interface for user to remote create and manage multiple HDDL-S pipelines and also get the processing result.


Hardware requirement:
	Host Board:	HDDL-S Module with 24 MYX cores
	CPU:            Intel® Xeon® E-2176G Processor
	Memory:	        16 GB

Software requirement:
	OS:		Ubuntu 16.04.3
	Kernel:		4.14.20
	OpenVINO:	R5
	OpenCL SDK:	2017_7.0.0.2568_x64
	Gstreamer:	1.8.3
	OpenCV:		4.0.0-rc
	Node JS:	11.10.0
	Node WS:	6.1.2

0. Install packages
-------------------
	sudo apt-get install libusb-1.0-0-dev libudev-dev libssl-dev rpm cmake libboost-program-options1.58-dev libboost-thread1.58 libboost-filesystem1.58 git libelf-dev dkms libssl-dev
	sudo apt-get install gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-plugins-bad libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
	sudo apt-get install libeigen3-dev  libopenblas-dev liblapack-dev libdlib-dev
	sudo ln -sf /opt/intel/mediasdk/lib64/libva.so.2 /usr/lib/libva.so
	sudo ln -sf /opt/intel/mediasdk/lib64/libva-drm.so.2 /usr/lib/libva-drm.so

1. Install OpenVINO R5
----------------------
Install OpenVINO: https://software.intel.com/en-us/articles/OpenVINO-Install-Linux

Note: below steps is the least
1) Install external software dependencies
2) Install the Intel Distribution of OpenVINO toolkit core components 
3) Set the environment variables
   source /opt/intel/computer_vision_sdk/bin/setupvars.sh
4) Additional installation steps for Intel Processor Graphics (GPU) 

	cd /opt/intel/computer_vision_sdk/install_dependencies
	sudo ./install_NEO_OCL_driver.sh

	Add OpenCL users to the video group: 
	sudo usermod -a -G video USERNAME
		e.g. if the user running OpenCL host applications is foo, run: sudo usermod -a -G video foo

	Install 4.14 kernel using install_4_14_kernel.sh script and reboot into this kernel
	sudo ./install_4_14_kernel.sh

	If you use 8th Generation Intel processor, you will need to add:
		i915.alpha_support=1
	to the 4.14 kernel command line, in order to enable OpenCL functionality for this platform.

5) Additional Installation Steps for Intel Vision Accelerator Design with Intel Movidius VPUs

2. Install OpenCL SDK
---------------------
	sudo apt-get install dkms
	tar -xvf intel_sdk_for_opencl_2017_7.0.0.2568_x64.gz
	cd intel_sdk_for_opencl_2017_7.0.0.2568_x64
	sudo ./install_GUI.sh

3. Install json-c
-----------------
        sudo apt-get install git
        git clone https://github.com/json-c/json-c.git
        cd json-c
        git checkout f8c632f579c71012f9aca81543b880a579f634fc
        sudo apt-get install autoconf libtool
        sh autogen.sh
        ./configure && make && sudo make install

4. Install OpenCV_4.0.0-rc
--------------------------
	export CPLUS_INCLUDE_PATH=/opt/intel/mediasdk/include:$CPLUS_INCLUDE_PATH
	sudo ln -sf /opt/intel/opencl/SDK/include /opt/intel/opencl/include
	sudo ln -sf /opt/intel/mediasdk/include/va   /usr/local/include/va
	git clone https://github.com/opencv/opencv.git
	cd opencv && git checkout -b 4.0.0-rc 4.0.0-rc
	          or git checkout a6387c3012d5331798ffca2acdd8297f417101e4
	mkdir build && cd build
	cmake -DWITH_VA_INTEL=ON -DWITH_IPP=OFF -DWITH_CUDA=OFF -DOPENCV_GENERATE_PKGCONFIG=ON ..
		Note: make sure:
			--     VA:                          YES
			--     Intel VA-API/OpenCL:         YES (OpenCL: /opt/intel/opencl)
	make -j8
	sudo make install
	Note: OpenVINO has provided OpenCV libraries, but HDDL-S need VA support in OpenCV, so we must rebuild it. 
	          terminate called after throwing an instance of 'cv::Exception'
		  what():  OpenCV(4.0.1-openvino) /home/jenkins/workspace/OpenCV/OpenVINO/build/opencv/modules/core/src/va_intel.cpp:51:
                           error: (-6:Unknown error code -6) OpenCV was build without VA support (libva) in function 'initializeContextFromVA'


5. Set enviroment variables
---------------------------
	source /opt/intel/computer_vision_sdk/bin/setupvars.sh
	export LD_LIBRARY_PATH=/usr/local/lib:/opt/intel/computer_vision_sdk_2018.5.445/deployment_tools/inference_engine/external/hddl/lib:$LD_LIBRARY_PATH
	export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/intel/mediasdk/lib64/pkgconfig
	export LD_LIBRARY_PATH=/opt/intel/mediasdk/lib64:/opt/intel/computer_vision_sdk/inference_engine/lib/ubuntu_16.04/intel64:/opt/intel/computer_vision_sdk_2018.5.445/deployment_tools/inference_engine/external/omp/lib:/usr/lib/x86_64-linux-gnu/gstreamer-1.0:$LD_LIBRARY_PATH
	export HDDLS_CVDL_KERNEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
	export PATH=$PATH:/opt/intel/mediasdk/bin/

6. Build from source code
-------------------------
        sudo apt-get install libgstreamer-plugins-bad1.0-dev  libx11-xcb-dev
	1).HDDL-S pipe/plugins, which are C/C++ 
		mkdir build && cd build
		cmake ..
		make -j8
		sudo make install
	2). HDDL-S server/client, which are NodeJS 
		It need not build, run it directly 

7. Setup HDDL-S Server
----------------------
	sudo apt-get install nodejs-legacy npm
	npm config set proxy <proxy>
	sudo npm install -g n
	sudo n stable
	cd hddls_server_controller_receiver
	npm install

8. Generate and install certifiate files
----------------------------------------
	Step 1: 
		Please refer to certificate_create_explanation.md to generate certificate files.
	Step 2: 
		Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'controller/client_cert'
		Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'receiver/client_cert'
		Copy "ca-crt.pem server-crt.pem server-key.pem" into 'server/server_cert'
		Copy 'client1.crl' into 'server_cert'
		Copy 'server.crl' into 'client_cert'
	Step 3:
		Change all pem files mode to be 400
		chmod 400 client_cert/*.pem
		chmod 400 server_cert/*.pem
		
		Change client_cert and server_cert mode to be 700
		chmod 700 client_cert
		chmod 700 server_cert

9. Run HDDL-S Server
---------------------
	cd hddls_server_controller_receiver/server
	export HDDLS_CVDL_MODEL_PATH=`pwd`/models
	node hddls-server.js

	Note:
		make sure models directory mode is 700
		make sure models/xxx/*.bin mode is 600
		make sure server_cert/*.pem mode is 400

10. Setup HDDL-S client in host machine
--------------------------------------
   10.1 Prepare environment

	sudo apt-get install nodejs-legacy npm
	npm config set proxy <proxy>
	sudo npm install -g n
	sudo n stable
	npm install

	Install certificate files, which is got from server:
		Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'controller/client_cert'
		Copy "ca-crt.pem client1-crt.pem client1-key.pem" into 'receiver/client_cert'
		Copy 'client1.crl' into 'server_cert'

   10.2 Run receiver client

	cd receiver
	node result_receiver.js

   10.3 Run controller client

        Open a new terminal, and run below command:
	cd hddls_server_controller_receiver/controller
	node controller_cli.js
		--Please choose server by ip: <server IP>:8445
		--Please type model update command:
			m <models directory>
	Note: Choose the models directory that will be updated into hddls-server
		You can type "models", which is the same with: HDDLS_CVDL_MODEL_PATH

       Note:
	1). Make sure client_cert/*.pem mode is 400
	2). Command format:
		help                          commanders that you can use.
		c <create.json>               create pipeslines
		p <property.json> <pipe_id>   set pipeslines property
		d <destroy.json>  <pipe_id>   destroy pipeslines
		pipe                          display pipes belonging to the very client
		m <models directory>          display model info
		q                             exit client.

	3). We need put the model files into a specified directory, and put the subdirectory the same name with model


11. Others
----------
	A. How to deploy customer models
		Step 1: implement libxxxalgo.so as customer guide
		Step 2: copy model IR files into <HDDLS_CVDL_MODEL_PATH>/<model_name>
		Step 3: register this customer models
				command: registeralgo -a <model_name>
		Step 4: edit create_xxx.json and add <model_name> into algopipeline property
		Step 5: controller_client send create pipeline command with create_xxx.json

		For an example: see hddls_server/models/example
			run command: registeralgo -a example
	B. Dont't remove the files in hddls_server/models
		The hddlspipe will read models file from this directory.

	C. Maybe it will fail to run h265 video stream, it was caused by cannot find libmfx_hevcd_hw64.so in /opt/intel/mediasdk/lib64/mfx/
		It was mediasdk driver cfg issues.
		There is a WA:
			cd /opt/intel/mediasdk/lib64
			sudo ln -sf ../plugins mfx
	D. How to run SRTP stream source
		Step 1: Setup SRTP server as below:
			launch-1.0 filesrc location=./1600x1200_concat.264 ! h264parse ! rtph264pay ! 'application/x-rtp, payload=(int)96, ssrc=(uint)114879' ! srtpenc key="012345678901234567890123456789012345678901234567890123456789" ! udpsink host=10.239.67.158 port=5000
			Note: Host is for the receiver IP, you can run this command in any other machine
		Step 2: Set json file at controller client
			Please refer: hddls_server_controller_receiver/controller/json_file/create_srtp.json

        Note: GST SRTP plugin encryption & authentication
			a.	Encryption (properties rtp-cipher and rtcp-cipher): AES_ICM 256 bits (maximum security)
			b.	Authentication (properties rtp-auth and rtcp-auth): HMAC_SHA1 80 bits (default, maximum protection)
            For example: srtp-cipher=(string)aes-256-icm
			             srtp-auth=(string)hmac-sha1-80
						 srtcp-cipher=(string)aes-256-icm
						 srtcp-auth=(string)hmac-sha1-80

