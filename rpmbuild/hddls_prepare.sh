#!/bin/bash

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

source /opt/intel/computer_vision_sdk/bin/setupvars.sh
export HDDL_INSTALL_DIR=/usr/local
export LD_LIBRARY_PATH=/usr/local/lib

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/intel/mediasdk/lib64/pkgconfig
export LD_LIBRARY_PATH=/opt/intel/mediasdk/lib64:/usr/local/lib:/opt/intel/computer_vision_sdk/inference_engine/lib/ubuntu_16.04/intel64:/opt/intel/computer_vision_sdk_2018.5.445/deployment_tools/inference_engine/external/omp/lib:/usr/lib/x86_64-linux-gnu/gstreamer-1.0:$LD_LIBRARY_PATH
export HDDLS_CVDL_KERNEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
export PATH=$PATH:/opt/intel/mediasdk/bin/
export OPENCV_OPENCL_BUFFERPOOL_LIMIT=0
#echo "HDDL-S prepare is done!"
echo ""
#export HDDLS_CVDL_MODEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models
