#!/bin/bash

git clone https://github.com/json-c/json-c.git
cd json-c
git checkout f8c632f579c71012f9aca81543b880a579f634fc
./autogen.sh
./configure && make && sudo make install
cd ..
rm -rf json-c

export CPLUS_INCLUDE_PATH=/opt/intel/mediasdk/include:$CPLUS_INCLUDE_PATH
export LD_LIBRARY_PATH=/opt/intel/mediasdk/lib64:$LD_LIBRARY_PATH
sudo unlink /usr/local/include/va
sudo ln -sf /opt/intel/mediasdk/include/va   /usr/local/include/va
sudo ln -sf /opt/intel/mediasdk/lib64/libva.so.2 /usr/lib/libva.so
sudo ln -sf /opt/intel/mediasdk/lib64/libva-drm.so.2 /usr/lib/libva-drm.so
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.3.0-openvino
rm -rf build
mkdir build
cd build
cmake -DWITH_VA_INTEL=ON -DWITH_IPP=OFF -DWITH_CUDA=OFF -DOPENCV_GENERATE_PKGCONFIG=ON ..
make -j16
sudo make install
cd ../..
rm -rf opencv

sudo -E apt install libeigen3-dev libdlib-dev libudev-dev
