.PHONY: all clean

SYS_INC_DIR := $(PREFIX)/usr/local/include
SYS_LIB_DIR := $(PREFIX)/usr/local/lib

all:
	@$(MAKE) -C gst-lib
	@$(MAKE) -C gst

clean:
	@$(MAKE) clean -C gst-lib
	@$(MAKE) clean -C gst

install:
	@cp gst/libgstcvdlfilter.so $(SYS_LIB_DIR)/gstreamer-1.0/.
	@cp gst/libgstresconvert.so $(SYS_LIB_DIR)/gstreamer-1.0/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdl
	@mkdir -p $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-lib/ocl/kernels $(SYS_LIB_DIR)/libgstcvdl/
	@rm -rf $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@ln -s $(SYS_LIB_DIR)/gstreamer-1.0/libgstcvdlfilter.so $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@rm -rf $(SYS_LIB_DIR)/libgstresconvert.so
	@ln -s $(SYS_LIB_DIR)/gstreamer-1.0/libgstresconvert.so $(SYS_LIB_DIR)/libgstresconvert.so
	@rm -rf $(SYS_INC_DIR)/gstcvdl
	@mkdir -p $(SYS_INC_DIR)/gstcvdl
	@cp gst-lib/ocl/oclcommon.h $(SYS_INC_DIR)/gstcvdl/
