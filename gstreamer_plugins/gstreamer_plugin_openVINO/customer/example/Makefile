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

SYS_INC_DIR := /usr/include
SYS_LIB_DIR := /usr/lib/x86_64-linux-gnu


LIB_PATH := -L$(SYS_LIB_DIR)
##LDFLAGS := --sysroot=$(PREFIX) -shared -fPIC -Wl,-no-as-needed $(LIB_PATH)
LDFLAGS := --sysroot=$(PREFIX) -shared -fPIC

CFLAGS  := -g -Wall -fPIC -std=c++11 -pthread
LIBS    := 

DIR_INC := .
DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .
LIB_OCL_PATH := .

SRCS = example.cpp

LIB_NAME    := libalgoexample
SHARE_LIB   := $(LIB_OCL_PATH)/$(LIB_NAME).so
DYNAMIC_LIB := $(LIB_OCL_PATH)/$(LIB_NAME).a

INC := -I$(DIR_INC)  -I$(SYS_INC_DIR)/gstcvdl
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(SHARE_LIB)  $(OBJS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a $(LIB_OCL_PATH)/*.so
