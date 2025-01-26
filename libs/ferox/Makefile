ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

CFLAGS += -Iinclude -Isrc -Isrc/external

LIB_FEROX_OBJS = src/broadphase.o \
	src/collision.o \
	src/dynamics.o \
	src/geometry.o \
	src/timer.o \
	src/utils.o \
	src/world.o

FEROX = libferox.a
BUILD_DIR = $(ROOT_DIR)/build/lib

$(FEROX): $(LIB_FEROX_OBJS)
	mkdir -p $(BUILD_DIR)
	$(AR) rcs $(BUILD_DIR)/$(FEROX) $(LIB_FEROX_OBJS)

clean:
	rm -f $(LIB_FEROX_OBJS)

