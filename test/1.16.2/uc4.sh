#!/bin/bash
rm -f .algocache.register
export HDDL_PLUGIN=GPU
gst-launch-1.0 filesrc location=../car-detection.mp4 ! qtdemux ! h264parse ! mfxh264dec ! cvdlfilter name=cvdlfilter0 algopipeline="vehicle-license-plate-detection-barrier-0106 ! vehicle-attributes-recognition-barrier-0039" ! resconvert name=resconvert0 resconvert0.src_txt ! ipcsink name=ipcsink_txt location=metadata.json resconvert0.src_pic ! mfxjpegenc ! ipcsink name=ipcsink_pic
