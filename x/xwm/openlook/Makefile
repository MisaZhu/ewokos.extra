TASK_OBJS = xwm.o OpenLookWM.o

SW_ROOT_DIR=../../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

LIB_DIR = $(SYS_ROOT_DIR)/build/$(HW)/lib

TASK = xwm_openlook
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/sbin/x

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(LIB_DIR)/libx++.a \
		$(LIB_DIR)/libx.a \
		$(LIB_DIR)/libfont.a \
		$(LIB_DIR)/libgraph.a \
		$(LIB_DIR)/libcxx.a 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH)  $(EWOK_LIBC)  -lcxx

clean:
	rm -f $(TASK_OBJS)
