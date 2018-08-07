GXX := g++
AR := ar

SYS_INC_DIR := $(PREFIX)/usr/include

ARFLAGS := -rc
CFLAGS  := -g -Wall -fpic

SRCS :=  log.cpp
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

DIR_SRC := .
DIR_OBJ := .
DIR_LIB := .

LIB_NAME    := libocl_common
DYNAMIC_LIB := $(LIB_NAME).a

all: $(OBJS)
	$(AR) $(ARFLAGS) $(DIR_LIB)/$(DYNAMIC_LIB) $(OBJS)

$(OBJS): $(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(GXX) $(CFLAGS) -I$(SYS_INC_DIR) -o $@ -c $<

clean:
	rm -rf $(DIR_OBJ)/*.o $(DIR_LIB)/*.so $(DIR_LIB)/*.a
