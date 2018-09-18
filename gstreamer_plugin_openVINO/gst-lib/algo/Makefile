GXX := g++
AR := ar

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
DRM_CFLAGS  := -I$(SYS_INC_DIR)/drm
VA_CFLAGS   :=  $(shell pkg-config --cflags libva)
OPENCL_CFLAGS := -I$(INTELOPENCLSDK)/include/
IE_CFLAGS   := -I${INTEL_CVSDK_DIR}/inference_engine/include

#OPENCV_CFLAGS := -I${INTEL_CVSDK_DIR}/opencv/include
#OPENCV_LIBS := -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_shape
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv)
OPENCV_LIBS := $(shell pkg-config --libs opencv)

IE_LIBS := -L${INTEL_CVSDK_DIR}/inference_engine/lib/ubuntu_16.04/intel64 -linference_engine

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic -std=c++11 $(GLIB_CFLAGS) $(GST_CFLAGS) $(DRM_CFLAGS) $(OPENCL_CFLAGS) $(IE_CFLAGS) $(OPENCV_CFLAGS) $(VA_CFLAGS)

DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .

SRC := $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, %.o, ${SRC})

LIB_NAME    := libcvdl_algo
SHARE_LIB   := $(LIB_NAME).so
DYNAMIC_LIB := $(LIB_NAME).a

INC := -I../ -I$(SYS_INC_DIR)

all: $(OBJ)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJ)

$(OBJ): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
