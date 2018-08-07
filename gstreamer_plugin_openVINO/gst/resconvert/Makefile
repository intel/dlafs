GXX := gcc
LD := gcc

# export GST_CFLAGS = $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
# export GST_LDFLAGS = $(shell pkg-config --libs glib-2.0)
# export DRM_CFLAGS = $(shell pkg-config --cflags libdrm)

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib
OPENCL_DIR  := $(PREFIX)/opt/intel/opencl
IE_DIR      := $(PREFIEX)/usr/local/include

#GLIB_CFLAGS := -I$(SYS_INC_DIR)/glib-2.0 -I$(SYS_LIB_DIR)/glib-2.0/include
#GST_CFLAGS  := -I$(SYS_INC_DIR)/gstreamer-1.0 -I$(SYS_LIB_DIR)/gstreamer-1.0/include
GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
DRM_CFLAGS  := -I$(SYS_INC_DIR)/libdrm
OPENCL_CFLAGS := -I$(OPENCL_DIR)/include/
IE_CFLAGS   := -I$(IE_DIR)

OPENCV_CFLAGS := $(shell pkg-config --cflags opencv)
OPENCV_LIBS   := $(shell pkg-config --libs opencv)
IE_LIBS := -L/usr/local/lib/ubuntu_16.04/intel64 -linference_engine

LIB_PATH := -L$(SYS_LIB_DIR) -L$(OPENCL_DIR) $(OPENCV_LIBS) $(IE_LIBS)
LDFLAGS := --sysroot=$(PREFIX) -shared -fPIC -Wl,-no-as-needed -lOpenCL -ldrm -ldrm_intel -lva -lva-drm -lglib-2.0 -lgstallocators-1.0 -lgstvideo-1.0 $(LIB_PATH)

CFLAGS  := -g -Wall -fpic -pthread $(GLIB_CFLAGS) $(GST_CFLAGS) $(DRM_CFLAGS) $(OPENCL_CFLAGS) $(IE_CFLAGS) $(OPENCV_CFLAGS)
LIBS    := $(OPENCV_LIBS) $(IE_LIBS)

DIR_INC := .
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .
LIB_OCL_PATH := ..
GST_LIBS_DIR := ../../gst-lib

SRCS = resconvert.c resmemory.c respool.c plugin.c

LIB_NAME    := libgstresconvert
SHARE_LIB   := $(LIB_OCL_PATH)/$(LIB_NAME).so
DYNAMIC_LIB := $(LIB_OCL_PATH)/$(LIB_NAME).a

INC := -I$(DIR_INC) -I$(GST_LIBS_DIR) -I$(GST_LIBS_DIR)/interface/ -I$(SYS_INC_DIR)
OBJS := $(patsubst %.c, %.o, $(SRCS))
GST_LIBS := $(GST_LIBS_DIR)/common/libocl_common.a $(GST_LIBS_DIR)/ocl/libocl_ocl.a $(GST_LIBS_DIR)/algo/libcvdl_algo.a $(GST_LIBS_DIR)/vpp/libocl_vpp.a

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(DIR_LIB)/libgstresconvertfilter.so $(OBJS)
	$(LD) $(LDFLAGS) -o $(SHARE_LIB) -Wl,--whole-archive $(GST_LIBS) $(OBJS) -Wl,--no-whole-archive $(LIBS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.c
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a $(LIB_OCL_PATH)/*.so

