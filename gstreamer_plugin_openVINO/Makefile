.PHONY: all clean

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

all:
	@$(MAKE) -C gst-libs
	@$(MAKE) -C gst
	@echo "Success to build cvdl gst plugins!"
clean:
	@$(MAKE) clean -C gst-libs
	@$(MAKE) clean -C gst
	@echo "Clean is done"

install:
	@cp libgstcvdlfilter.so $(SYS_LIB_DIR)/gstreamer-1.0/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdl
	@mkdir -p $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-libs/ocl/kernels $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-libs/models $(SYS_LIB_DIR)/libgstcvdl/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@ln -s $(SYS_LIB_DIR)/gstreamer-1.0/libgstcvdlfilter.so $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@rm -rf $(SYS_INC_DIR)/gstcvdl
	@mkdir -p $(SYS_INC_DIR)/gstcvdl
	@cp gst-libs/ocl/oclcommon.h $(SYS_INC_DIR)/gstcvdl/.
	@cp gst-libs/ws/wsclient.h $(SYS_INC_DIR)/gstcvdl/.
	@echo "ocl kernel are installed into " $(SYS_LIB_DIR)/libgstcvdl
	@echo "cvdl model are installed into" $(SYS_LIB_DIR)/libgstcvdl
	@echo ""
	@echo "Success to install cvdl plugins into "$(SYS_LIB_DIR)/gstreamer-1.0
	@echo ""
