GXX := g++
AR := ar

#export GST_CFLAGS = $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib
OPENCL_DIR := $(PREFIX)/opt/intel/opencl

#GLIB_CFLAGS := -I$(SYS_INC_DIR)/glib-2.0 -I$(SYS_LIB_DIR)/glib-2.0/include
#GST_CFLAGS  := -I$(SYS_INC_DIR)/gstreamer-1.0 -I$(SYS_LIB_DIR)/gstreamer-1.0/include
DRM_CFLAGS  := -I$(SYS_INC_DIR)/libdrm
OPENCL_CFLAGS := -I$(OPENCL_DIR)/include/
GST_CFLAGS    := $(shell pkg-config --cflags gstreamer-1.0)



ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic $(GLIB_CFLAGS) $(GST_CFLAGS) $(DRM_CFLAGS) $(OPENCL_CFLAGS)

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
