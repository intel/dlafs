#!/bin/bash
rm -f .algocache.register
export HDDL_PLUGIN=HDDL
GST_LAUNCH=/opt/intel/openvino_2020.4.287/data_processing/gstreamer/bin/gst-launch-1.0
${GST_LAUNCH} filesrc location=../1600x1200.mp4 ! qtdemux ! h264parse ! mfxh264dec ! cvdlfilter name=cvdlfilter0 algopipeline="mobilenetssd ! tracklp ! lprnet" ! resconvert name=resconvert0 resconvert0.src_txt ! ipcsink name=ipcsink_txt location=metadata.json resconvert0.src_pic ! mfxjpegenc ! ipcsink name=ipcsink_pic
