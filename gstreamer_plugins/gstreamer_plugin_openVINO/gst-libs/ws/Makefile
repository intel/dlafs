#Copyright (C) 2018 Intel Corporation
#
#SPDX-License-Identifier: LGPL-2.1-only
#
#This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License as published by the Free Software Foundation;
# version 2.1.
#
#This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this library;
# if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

GXX := g++
AR := ar

SYS_INC_DIR := $(PREFIX)/usr/include
SYS_LIB_DIR := $(PREFIX)/usr/lib

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GST_CFLAGS  := $(shell pkg-config --cflags gstreamer-1.0)

ARFLAGS := -rc
CFLAGS  := -Wall -fpic -std=c++11 $(GLIB_CFLAGS) $(GST_CFLAGS)
CFLAGS += -fstack-protector-strong -fPIE -fPIC -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security

ifdef DEBUG
	CFLAGS += -g
endif

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
