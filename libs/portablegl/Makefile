SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_PORTABLEGL_OBJS = src/portablegl.o

PORTABLEGL = $(BUILD_DIR)/lib/libportablegl.a

$(PORTABLEGL): $(LIB_PORTABLEGL_OBJS)
	$(AR) rcs $(PORTABLEGL) $(LIB_PORTABLEGL_OBJS)
	cp -r include/* $(BUILD_DIR)/include

clean:
	rm -f $(LIB_PORTABLEGL_OBJS)
	rm -f $(PORTABLEGL)
