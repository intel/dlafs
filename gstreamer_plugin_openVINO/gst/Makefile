GXX := gcc
LD := gcc

SYS_INC_DIR := /usr/include/
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu/

GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
DRM_CFLAGS  := -I$(SYS_INC_DIR)/drm
VA_CFLAGS   :=  $(shell pkg-config --cflags libva)
OPENCL_CFLAGS := -I$(INTELOPENCLSDK)/include/
IE_CFLAGS   := -I${INTEL_CVSDK_DIR}/inference_engine/include

#OPENCV_CFLAGS := -I${INTEL_CVSDK_DIR}/opencv/include
#OPENCV_LIBS := -L${INTEL_CVSDK_DIR}/opencv/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_shape
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv)
OPENCV_LIBS := $(shell pkg-config --libs opencv)

IE_LIBS := -L${INTEL_CVSDK_DIR}/inference_engine/lib/ubuntu_16.04/intel64 -linference_engine

LIB_PATH := -L$(SYS_LIB_DIR) -L$(INTELOPENCLSDK) $(OPENCV_LIBS) $(IE_LIBS) -L$(SYS_LIB_DIR)
# -ldrm -ldrm_intel
LDFLAGS := --sysroot=$(PREFIX) -shared -fPIC -Wl,-no-as-needed -lOpenCL -lglib-2.0 -lgstallocators-1.0 -lgstvideo-1.0 $(LIB_PATH)

CFLAGS  := -g -Wall -fpic -pthread $(GLIB_CFLAGS) $(GST_CFLAGS) $(DRM_CFLAGS) $(OPENCL_CFLAGS) $(IE_CFLAGS) $(OPENCV_CFLAGS) $(VA_CFLAGS)
LIBS    := $(OPENCV_LIBS) $(IE_LIBS) -luWS -lcrypto -lssl

DIR_INC := .
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .
LIB_OCL_PATH := ..
GST_LIBS_DIR := ../gst-lib

SRCS = cvdlfilter.c resconvert.c resmemory.c respool.c wssink.c plugin.c

LIB_NAME    := libgstcvdlfilter
SHARE_LIB   := $(LIB_OCL_PATH)/$(LIB_NAME).so
DYNAMIC_LIB := $(LIB_OCL_PATH)/$(LIB_NAME).a

INC := -I$(DIR_INC) -I$(GST_LIBS_DIR) -I$(GST_LIBS_DIR)/interface/ -I$(SYS_INC_DIR)
OBJS := $(patsubst %.c, %.o, $(SRCS))
GST_LIBS := $(GST_LIBS_DIR)/common/libocl_common.a $(GST_LIBS_DIR)/ocl/libocl_ocl.a $(GST_LIBS_DIR)/algo/libcvdl_algo.a $(GST_LIBS_DIR)/vpp/libocl_vpp.a $(GST_LIBS_DIR)/ws/libws_client.a

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(SHARE_LIB) -Wl,--whole-archive $(GST_LIBS) $(OBJS) -Wl,--no-whole-archive $(LIBS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.c
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a $(LIB_OCL_PATH)/*.so
