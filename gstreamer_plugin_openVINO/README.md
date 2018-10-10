1. build
   make
   sudo make install

2. Prepare to run

export CVDL_KERNEL_DIR=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
export CVDL_CLASSIFICATION_MODEL_FULL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models/vehicle_classify/carmodel_fine_tune_1062_bn_iter_370000.xml
export CVDL_DETECTION_MODEL_FULL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models/vehicle_detect/yolov1-tiny.xml


2. Test to run local file

gst-launch-1.0 filesrc location=<file> ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res \
res.src_pic ! mfxjpegenc ! wssink name=ws \
res.src_txt ! ws.

