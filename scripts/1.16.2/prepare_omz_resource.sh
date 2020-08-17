#!/bin/bash
export dlafs_root=`realpath ../..`
remote=https://download.01.org/opencv/2020/openvinotoolkit/2020.4/open_model_zoo/models_bin/1/
for model in vehicle-attributes-recognition-barrier-0039 vehicle-license-plate-detection-barrier-0106
do
    rm -rf $dlafs_root/test/models/$model
    mkdir -p $dlafs_root/test/models/$model
    cd $dlafs_root/test/models/$model
    wget $remote/$model/FP16/$model.bin
    wget $remote/$model/FP16/$model.xml
    cd -
done

cd $dlafs_root/test
wget https://github.com/intel-iot-devkit/sample-videos/raw/master/car-detection.mp4
