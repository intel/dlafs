#!/bin/bash

source /opt/intel/computer_vision_sdk/bin/setupvars.sh
export HDDL_INSTALL_DIR=/usr/local
export LD_LIBRARY_PATH=/usr/local/lib
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/intel/mediasdk/lib64/pkgconfig
export LD_LIBRARY_PATH=/opt/intel/mediasdk/lib64:/usr/local/lib:/usr/local/lib/ubuntu_16.04/intel64:$LD_LIBRARY_PATH
export CVDL_KERNEL_DIR=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
export CVDL_CLASSIFICATION_MODEL_FULL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models/vehicle_classify/carmodel_fine_tune_1062_bn_iter_370000.xml
export CVDL_DETECTION_MODEL_FULL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models/vehicle_detect/yolov1-tiny.xml
export PATH=$PATH:/opt/intel/mediasdk/bin/

