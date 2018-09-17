GXX := g++
AR := ar

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu

GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)
DRM_CFLAGS  := -I$(SYS_INC_DIR)/drm
OPENCL_CFLAGS := -I$(INTELOPENCLSDK)/include/
OPENCV_CFLAGS := -I${INTEL_CVSDK_DIR}/opencv/include

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic $(GLIB_CFLAGS) $(GST_CFLAGS) $(DRM_CFLAGS) $(OPENCL_CFLAGS) $(OPENCV_CFLAGS)

DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .

SRC := $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, %.o, ${SRC})

LIB_NAME    := libocl_ocl
SHARE_LIB   := $(LIB_NAME).so
DYNAMIC_LIB := $(LIB_NAME).a

INC := -I../ -I$(SYS_INC_DIR)

all: $(OBJ)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJ)

$(OBJ): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
