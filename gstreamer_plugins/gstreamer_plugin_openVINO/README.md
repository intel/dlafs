1. How to build
   Method 1: Makefile
      make -j8
   	  sudo make install

   Method 2: CMake
     cd <top dir>
     mkdir build
	 cd build
	 cmake ..
	 make -j8
	 sudo make install


2. Test to run local file

 1). jpeg data and meta data
 gst-launch-1.0 filesrc location=<mp4 file>  ! qtdemux  ! h264parse  ! mfxh264dec  ! cvdlfilter name=cvdlfilter0 algopipeline="yolov1tiny ! opticalflowtrack ! googlenetv2"  ! resconvert name=resconvert0  resconvert0.src_pic ! mfxjpegenc ! wssink name=wssink0 wsclientid=1 wssuri=<server socket uri>   resconvert0.src_txt ! wssink0.

 2). only meta data
  gst-launch-1.0 filesrc location=<mp4 file>  ! qtdemux  ! h264parse  ! mfxh264dec  ! cvdlfilter name=cvdlfilter0 algopipeline="yolov1tiny ! opticalflowtrack ! googlenetv2"  ! resconvert name=resconvert0  resconvert0.src_txt !  wssink name=wssink0 wsclientid=1 wssuri=<server socket uri>
