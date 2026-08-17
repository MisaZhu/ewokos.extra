SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = $(ARCH)/3ddemo.o

EXTRA_LIBS_DIR = $(SYS_ROOT_DIR)/build/$(HW)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include -I $(EXTRA_LIBS_DIR)/include/glcommon
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib

GLCOMMON_LIB = $(EXTRA_LIBS_DIR)/lib/libglcommon.a \
	$(EXTRA_LIBS_DIR)/lib/libportablegl.a

TASK = 3ddemo
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) $(GLCOMMON_LIB) \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewoksys.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewokstl.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libfont.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libgraph.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libwidget++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libopenlibm.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libcxx.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) $(GLCOMMON_LIB) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
