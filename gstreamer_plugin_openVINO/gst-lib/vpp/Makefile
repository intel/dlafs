GXX := g++
AR := ar

# export GST_CFLAGS = $(shell pkg-config --cflags glib-2.0)

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GST_CFLAGS  := $(shell pkg-config --cflags gstreamer-1.0)
#GLIB_CFLAGS := -I$(SYS_INC_DIR)/glib-2.0 -I$(SYS_LIB_DIR)/glib-2.0/include
#GST_CFLAGS  := -I$(SYS_INC_DIR)/gstreamer-1.0 -I$(SYS_LIB_DIR)/gstreamer-1.0/include

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic $(GLIB_CFLAGS) $(GST_CFLAGS)

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

INC := -I$(DIR_INC) -I$(OPENCL_DIR)/include/ -I$(SYS_INC_DIR) -I../ -I./

all: $(OBJS)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
