# Copyright (C) 2018 Intel Corporation
# 
# SPDX-License-Identifier: MIT
#
#  MIT License
#
#  Copyright (c) 2019 Intel Corporation
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy of
#  this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation the rights to
#  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
#  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
#  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
#  AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
#  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

.PHONY: src
CXX := g++

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

#===========objects list=====================
ROOT_DIR = $(shell pwd)
SRC_FILES = $( $(wildcard $(ROOT_DIR)/src/*.c)  )
export OBJECTS = $(patsubst %.c, %.o, $(SRC_FILES))

#==================options==================
JSON_CFLAGS :=  $(shell pkg-config --cflags glib-2.0) 
LOCAL_CFLAGS  =  -I$(ROOT_DIR)/src -I$(SYS_INC_DIR)/gstcvdl
GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
export CXXFLAGS = -pthread -Wall -Wno-deprecated-declarations $(JSON_CFLAGS)  $(GST_CFLAGS)  $(LOCAL_CFLAGS)
CXXFLAGS += -g -O0

JSON_C_LIBS := $(shell pkg-config --libs json-c)
GST_LIBS :=  $(shell pkg-config --libs gstreamer-1.0)
LIB_PATH = -L$(SYS_LIB_DIR)  -L$(SYS_LIB_DIR)/gstreamer-1.0 $(JSON_C_LIBS) $(GST_LIBS)
export LDFLAGS = $(LIB_PATH) -lgstvideo-1.0  -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0  -lgstapp-1.0 -ljson-c -lgstcvdlfilter -lstdc++

LDFLAGS += -z noexecstack -z relro -z now -pie
CXXFLAGS += -fstack-protector-strong -fPIE -fPIC -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security

export DEFINES := -DDEBUG

#==================rules=====================
#%.o:%.c
#	@$(CXX) $< -o $@ -c $(CXXFLAGS) $(DEFINES)

all:
	@$(MAKE) -C src
	@$(MAKE) -C tools
	@$(MAKE) -C test

clean:
	@$(MAKE) clean -C src
	@$(MAKE) clean -C tools
	@$(MAKE) clean -C test
install:
	@cp src/hddlspipes /usr/local/bin/.
	@cp tools/registeralgo /usr/local/bin/.
	@cp test/hddlspipestest /usr/local/bin/.
uninstall:
	@rm -rf /usr/local/bin/hddlspipes
	@rm -rf /usr/local/bin/registeralgo
	@rm -rf /usr/local/bin/hddlspipestest
