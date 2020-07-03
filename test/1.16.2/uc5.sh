#!/bin/bash
rm -f .algocache.register
export HDDL_PLUGIN=HDDL
gst-launch-1.0 filesrc location=../1600x1200.mp4 ! qtdemux ! h264parse ! mfxh264dec ! cvdlfilter name=cvdlfilter0 algopipeline="yolov1tiny ! opticalflowtrack ! googlenetv2" ! resconvert name=resconvert0 resconvert0.src_txt ! ipcsink name=ipcsink_txt location=metadata.json resconvert0.src_pic ! mfxjpegenc ! ipcsink name=ipcsink_pic
