GXX := g++
AR := ar

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GST_CFLAGS  := $(shell pkg-config --cflags gstreamer-1.0)

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic -std=c++11 $(GLIB_CFLAGS) $(GST_CFLAGS)

DIR_INC := .
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

LIB_NAME    := libws_client
SHARE_LIB   := $(LIB_NAME).so
DYNAMIC_LIB := $(LIB_NAME).a

SYS_INC_DIR := $(PREFIX)/usr/include

INC := -I$(DIR_INC) -I$(SYS_INC_DIR) -I../ -I./

all: $(OBJS)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
