.PHONY: all clean

SYS_INC_DIR := /usr/include/
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu/

all:
	@$(MAKE) -C gst-lib
	@$(MAKE) -C gst

clean:
	@$(MAKE) clean -C gst-lib
	@$(MAKE) clean -C gst

install:
	@cp libgstcvdlfilter.so $(SYS_LIB_DIR)/gstreamer-1.0/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdl
	@mkdir -p $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-lib/ocl/kernels $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-lib/models $(SYS_LIB_DIR)/libgstcvdl/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@ln -s $(SYS_LIB_DIR)/gstreamer-1.0/libgstcvdlfilter.so $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@rm -rf $(SYS_INC_DIR)/gstcvdl
	@mkdir -p $(SYS_INC_DIR)/gstcvdl
	@cp gst-lib/ocl/oclcommon.h $(SYS_INC_DIR)/gstcvdl/
