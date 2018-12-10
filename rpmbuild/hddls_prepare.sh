#!/bin/bash

source /opt/intel/computer_vision_sdk/bin/setupvars.sh
export HDDL_INSTALL_DIR=/usr/local
export LD_LIBRARY_PATH=/usr/local/lib
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/intel/mediasdk/lib64/pkgconfig
export LD_LIBRARY_PATH=/opt/intel/mediasdk/lib64:/usr/local/lib:/usr/local/lib/ubuntu_16.04/intel64:$LD_LIBRARY_PATH
export HDDLS_CVDL_KERNEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
export HDDLS_CVDL_MODEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models
export PATH=$PATH:/opt/intel/mediasdk/bin/

