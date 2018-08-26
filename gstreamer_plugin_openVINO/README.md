Test to run local file

gst-launch-1.0 filesrc location=<file> ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res \
res.src_pic ! mfxjpegenc ! wssink name=ws \
res.src_txt ! ws.

