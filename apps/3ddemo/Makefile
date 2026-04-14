SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = 3ddemo.o

EXTRA_LIBS_DIR = $(SYS_ROOT_DIR)/build/$(HW)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include -I ../../libs/glcommon/include -I ../../libs/glcommon/include/glcommon -I ../../libs/portablegl/include -I ../../libs/portablegl/include/portablegl
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib

GLCOMMON_LIB = $(EXTRA_LIBS_DIR)/lib/libglcommon.a \
	$(EXTRA_LIBS_DIR)/lib/libportablegl.a

TASK = 3ddemo
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) $(GLCOMMON_LIB)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) $(GLCOMMON_LIB) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
