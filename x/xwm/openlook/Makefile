SW_ROOT_DIR=../../..
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = $(ARCH)/xwm.o $(ARCH)/OpenLookWM.o

LIB_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib

TASK = xwm_openlook
BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/rootfs/sbin/x

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(LIB_DIR)/libx++.a \
		$(LIB_DIR)/libx.a \
		$(LIB_DIR)/libfont.a \
		$(LIB_DIR)/libgraph.a \
		$(LIB_DIR)/libcxx.a \
		$(LIB_DIR)/libewoksys.a \
		$(LIB_DIR)/libewokstl.a \
		$(LIB_DIR)/libwidget++.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH)  $(EWOK_LIBC)  -lcxx

clean:
	rm -f $(TASK_OBJS)
