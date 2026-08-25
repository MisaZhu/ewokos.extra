SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = $(ARCH)/doom.o

TASK = doom
BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/rootfs/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libewoksys.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libewokstl.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libx++.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libx.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libfont.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libgraph.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libwidget++.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libopenlibm.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libcxx.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
