#!/bin/bash -e 

# HDDL-S depency packages installation:

: <<'COMMENT'
1. Install OpenVINO R3
Install OpenVINO: https://software.intel.com/en-us/articles/OpenVINO-Install-Linux
#source /opt/intel/computer_vision_sdk/bin/setupvars.sh

2. Install OpenCL
#cd /opt/intel/computer_vision_sdk/install_dependencies
#sudo ./install_NEO_OCL_driver.sh

Add OpenCL users to the video group: 
#sudo usermod -a -G video USERNAME
   e.g. if the user running OpenCL host applications is foo, run: sudo usermod -a -G video foo

Install 4.14 kernel using install_4_14_kernel.sh script and reboot into this kernel
#sudo ./install_4_14_kernel.sh

If you use 8th Generation Intel processor, you will need to add:
   i915.alpha_support=1
   to the 4.14 kernel command line, in order to enable OpenCL functionality for this platform.

3. Install OpenCL SDK
#sudo apt-get install dkms
#tar -xvf intel_sdk_for_opencl_2017_7.0.0.2568_x64.gz
#cd intel_sdk_for_opencl_2017_7.0.0.2568_x64
#./install_GUI.sh

4. Install HDDL-R
#sudo apt-get install libusb-1.0-0-dev libudev-dev libssl-dev rpm cmake libboost-program-options1.58-dev libboost-thread1.58 libboost-filesystem1.58 git libelf-dev

Install json-c
#git clone https://github.com/json-c/json-c.git
#cd json-c
#git checkout f8c632f579c71012f9aca81543b880a579f634fc
#sudo apt-get install autoconf libtool
#sh autogen.sh
#./configure && make && sudo make install

install HDDL rpm package:
#sudo rpm -ivh --nodeps Intel_Movidius_MyriadX_HDDL-R_Linux-01.02.12.B.36.3.rpm
#export HDDL_INSTALL_DIR=/usr/local
#export LD_LIBRARY_PATH=/usr/local/lib
#sudo usermod -a -G users,video $USER

5. Install uWebSocket
#sudo apt-get install libssl-dev
#git clone https://github.com/uNetworking/uWebSockets.git
#make && sudo make install

6. Install OpenCV
#sudo apt-get install libgtk2.0-dev pkg-config libgtkglext1-dev
#export CPLUS_INCLUDE_PATH=/opt/intel/mediasdk/include:$CPLUS_INCLUDE_PATH
#git clone https://github.com/opencv/opencv.git
#cd opencv && git checkout 6ffc48769ac60d53c4bd1913eac15117c9b1c9f7
#mkdir build && cd build
#cmake -DWITH_VA_INTEL=ON -DWITH_CUDA=OFF ..
#make -j8
#sudo make install
Note: OpenVINO has provided OpenCV libraries, but HDDL-S need VA support in OpenCV, so we must rebuild it. 

COMMENT


sudo apt-get install libusb-1.0-0-dev libudev-dev libssl-dev rpm cmake libboost-program-options1.58-dev libboost-thread1.58 libboost-filesystem1.58 git libelf-dev dkms libssl-dev
sudo apt-get install gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-plugins-bad libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install  libdrm-dev libudev-dev libgstreamer-plugins-bad1.0-dev libx11-xcb-dev libgles2-mesa-dev libgl1-mesa-dev libgtk2.0-dev pkg-config libgtkglext1-dev
sudo ln -sf /opt/intel/mediasdk/lib64/libva.so.2 /usr/lib/libva.so
sudo ln -sf /opt/intel/mediasdk/lib64/libva-drm.so.2 /usr/lib/libva-drm.so

./hddls_prepare.sh
