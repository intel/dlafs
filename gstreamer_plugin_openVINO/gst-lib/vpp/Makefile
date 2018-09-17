GXX := g++
AR := ar

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
OPENCL_CFLAGS := -I$(INTELOPENCLSDK)/include/
OPENCV_CFLAGS := -I${INTEL_CVSDK_DIR}/opencv/include/

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic $(GLIB_CFLAGS) $(GST_CFLAGS) $(OPENCV_CFLAGS)

DIR_INC := ../interface
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

LIB_NAME    := libocl_vpp
SHARE_LIB   := $(LIB_NAME).so
DYNAMIC_LIB := $(LIB_NAME).a

SYS_INC_DIR := $(PREFIX)/usr/include
OPENCL_DIR := $(PREFIX)/opt/intel/opencl

INC := -I$(DIR_INC) -I$(OPENCL_CFLAGS) -I$(SYS_INC_DIR) -I../ -I./

all: $(OBJS)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
