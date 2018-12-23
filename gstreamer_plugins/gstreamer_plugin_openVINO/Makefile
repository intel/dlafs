#Copyright (C) 2018 Intel Corporation
#
#SPDX-License-Identifier: LGPL-2.1-only
#
#This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License as published by the Free Software Foundation;
# version 2.1.
#
#This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this library;
# if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

.PHONY: all clean

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

all:
	@$(MAKE) -C gst-libs
	@$(MAKE) -C gst
	@$(MAKE) -C customer
	@echo "Success to build cvdl gst plugins!"
clean:
	@$(MAKE) clean -C gst-libs
	@$(MAKE) clean -C gst
	@$(MAKE) clean -C customer
	@echo "Clean is done"

install:
	@cp libgstcvdlfilter.so $(SYS_LIB_DIR)/gstreamer-1.0/.
	@rm -rf $(SYS_LIB_DIR)/libgstcvdl
	@mkdir -p $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r gst-libs/ocl/kernels $(SYS_LIB_DIR)/libgstcvdl/
	@cp -r models $(SYS_LIB_DIR)/libgstcvdl/.
	@cp gst-libs/resources/* $(SYS_LIB_DIR)/libgstcvdl/.
	@mkdir -p $(SYS_LIB_DIR)/libgstcvdl/models/example
	@cp customer/example/*.bin $(SYS_LIB_DIR)/libgstcvdl/models/example/.
	@cp customer/example/*.xml $(SYS_LIB_DIR)/libgstcvdl/models/example/.
	@cp customer/example/*.so $(SYS_LIB_DIR)/libgstcvdl/models/example/.
	#@rm -rf $(SYS_LIB_DIR)/libgstcvdlfilter.so
	#@ln -s $(SYS_LIB_DIR)/gstreamer-1.0/libgstcvdlfilter.so $(SYS_LIB_DIR)/libgstcvdlfilter.so
	@rm -rf $(SYS_INC_DIR)/gstcvdl
	@mkdir -p $(SYS_INC_DIR)/gstcvdl
	@cp gst-libs/ocl/oclcommon.h $(SYS_INC_DIR)/gstcvdl/.
	@cp gst-libs/ws/wsclient.h $(SYS_INC_DIR)/gstcvdl/.
	@cp gst-libs/algo/algoregister.h $(SYS_INC_DIR)/gstcvdl/.
	@cp gst-libs/algo/exinferdata.h $(SYS_INC_DIR)/gstcvdl/.
	@cp gst-libs/algo/exinferenceparser.h $(SYS_INC_DIR)/gstcvdl/.
	@echo "ocl kernel are installed into " $(SYS_LIB_DIR)/libgstcvdl
	@echo "cvdl model are installed into" $(SYS_LIB_DIR)/libgstcvdl
	@echo ""
	@echo "Success to install cvdl plugins into "$(SYS_LIB_DIR)/gstreamer-1.0
	@echo ""

uninstall:
	@rm -rf $(SYS_LIB_DIR)/libgstcvdl
	@rm -rf $(SYS_LIB_DIR)/gstreamer-1.0/libgstcvdlfilter.so
	@rm -rf $(SYS_INC_DIR)/gstcvdl
	@echo ""
	@echo "Success to uninstall cvdl plugins from "$(SYS_LIB_DIR)/gstreamer-1.0
	@echo ""

