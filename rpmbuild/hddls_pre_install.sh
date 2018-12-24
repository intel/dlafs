#!/bin/bash -e 

# Copyright (C) 2018 Intel Corporation
# 
# SPDX-License-Identifier: MIT
#
#  MIT License
#
#  Copyright (c) 2019 Intel Corporation
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy of
#  this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation the rights to
#  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
#  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
#  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
#  AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
#  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

# HDDL-S depency packages installation:

sudo apt-get install libusb-1.0-0-dev libudev-dev libssl-dev rpm cmake libboost-program-options1.58-dev libboost-thread1.58 libboost-filesystem1.58 git libelf-dev dkms libssl-dev
sudo apt-get install gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-plugins-bad libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install libeigen3-dev  libopenblas-dev liblapack-dev libdlib-dev
sudo apt-get install  libdrm-dev libudev-dev libgstreamer-plugins-bad1.0-dev libx11-xcb-dev libgles2-mesa-dev libgl1-mesa-dev libgtk2.0-dev pkg-config libgtkglext1-dev
sudo ln -sf /opt/intel/mediasdk/lib64/libva.so.2 /usr/lib/libva.so
sudo ln -sf /opt/intel/mediasdk/lib64/libva-drm.so.2 /usr/lib/libva-drm.so

./hddls_prepare.sh
