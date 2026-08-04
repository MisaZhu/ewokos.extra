SW_ROOT_DIR=../../..
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = $(ARCH)/xwm.o $(ARCH)/MacWM.o

LIB_DIR = $(SYS_ROOT_DIR)/build/$(HW)/lib

TASK = xwm_mac1984
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/sbin/x

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(LIB_DIR)/libx++.a \
		$(LIB_DIR)/libx.a \
		$(LIB_DIR)/libfont.a \
		$(LIB_DIR)/libgraph.a \
		$(LIB_DIR)/libcxx.a 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC)  -lcxx

clean:
	rm -f $(TASK_OBJS)
