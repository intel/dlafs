#!/bin/bash
export dlafs_root=`realpath ../..`
source /opt/intel/openvino_2020.3.194/bin/setupvars.sh
export PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig:$PKG_CONFIG_PATH
cd $dlafs_root
rm -rf build; mkdir build; cd build
cmake ..; make
sudo cp $dlafs_root/build/gstreamer_plugins/libgstmfx.so /opt/intel/openvino_2020.3.194/data_processing/gstreamer/lib/gstreamer-1.0
sudo cp $dlafs_root/build/gstreamer_plugins/gstreamer_plugin_openVINO/libgstcvdlfilter.so /opt/intel/openvino_2020.3.194/data_processing/gstreamer/lib/gstreamer-1.0
sudo cp $dlafs_root/build/gstreamer_pipeline/hddls_pipe_hal/test/hddlspipestest /opt/intel/openvino_2020.3.194/data_processing/gstreamer/bin
