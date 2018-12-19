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
export CXXFLAGS += -g -O0 -pthread -Wall -Wno-deprecated-declarations $(JSON_CFLAGS)  $(GST_CFLAGS)  $(LOCAL_CFLAGS)

JSON_C_LIBS := $(shell pkg-config --libs json-c)
GST_LIBS :=  $(shell pkg-config --libs gstreamer-1.0)
LIB_PATH = -L$(SYS_LIB_DIR)  -L$(SYS_LIB_DIR)/gstreamer-1.0 $(JSON_C_LIBS) $(GST_LIBS)
export LDFLAGS += $(LIB_PATH) -lgstvideo-1.0  -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0  -lgstapp-1.0 -ljson-c -lgstcvdlfilter -lstdc++

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
