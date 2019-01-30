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

gst-launch-1.0 filesrc location=<file> ! h264parse ! mfxh264dec ! cvdlfilter ! resconvert name=res \
res.src_pic ! mfxjpegenc ! wssink name=ws \
res.src_txt ! ws.

