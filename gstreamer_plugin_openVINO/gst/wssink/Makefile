GXX := gcc
LD := gcc

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib

GST_CFLAGS  := $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gstreamer-1.0)


LIB_PATH := -L$(SYS_LIB_DIR)
LDFLAGS := --sysroot=$(PREFIX) -shared -fPIC -Wl,-no-as-needed -lglib-2.0 -lgstallocators-1.0 -lgstvideo-1.0 $(LIB_PATH)

CFLAGS  := -g -Wall -fpic -pthread $(GST_CFLAGS)
LIBS    := 

DIR_INC := .
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .
LIB_OCL_PATH := ..
GST_LIBS_DIR := ../../gst-lib

SRCS = wssink.c

LIB_NAME    := libgstwssink
SHARE_LIB   := $(LIB_OCL_PATH)/$(LIB_NAME).so
DYNAMIC_LIB := $(LIB_OCL_PATH)/$(LIB_NAME).a

INC := -I$(DIR_INC) -I$(GST_LIBS_DIR) -I$(SYS_INC_DIR)
OBJS := $(patsubst %.c, %.o, $(SRCS))
GST_LIBS :=

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(SHARE_LIB) -Wl,--whole-archive $(GST_LIBS) $(OBJS) -Wl,--no-whole-archive $(LIBS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.c
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a $(LIB_OCL_PATH)/*.so

