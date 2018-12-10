1. build
   make
   sudo make install

2. Prepare to run

export HDDLS_CVDL_KERNEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/kernels
export HDDLS_CVDL_MODEL_PATH=/usr/lib/x86_64-linux-gnu/libgstcvdl/models


2. Test to run local file

gst-launch-1.0 filesrc location=<file> ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res \
res.src_pic ! mfxjpegenc ! wssink name=ws \
res.src_txt ! ws.

