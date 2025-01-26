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

$(FEROX): $(LIB_FEROX_OBJS)
	$(AR) rcs $(FEROX) $(LIB_FEROX_OBJS)

install:
	cp $(FEROX) $(SDK_DIR)/lib/
	cp -r include/* src/external/*.h $(SDK_DIR)/include/

clean:
	rm -f $(LIB_FEROX_OBJS)
	rm -f $(FEROX)

