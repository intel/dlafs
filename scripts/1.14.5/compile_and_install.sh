#!/bin/bash
export dlafs_root=`realpath ../..`
source /opt/intel/openvino_2020.3.194/bin/setupvars.sh
export PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig
cd $dlafs_root
rm -rf build; mkdir build; cd build
cmake ..; make
sudo cp $dlafs_root/build/gstreamer_plugins/libgstmfx.so /usr/lib/x86_64-linux-gnu/gstreamer-1.0
sudo cp $dlafs_root/build/gstreamer_plugins/gstreamer_plugin_openVINO/libgstcvdlfilter.so /usr/lib/x86_64-linux-gnu/gstreamer-1.0
