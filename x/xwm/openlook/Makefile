TASK_OBJS = xwm.o OpenLookWM.o

SW_ROOT_DIR=../../..
SYS_DIR=$(SW_ROOT_DIR)/../system
include $(SW_ROOT_DIR)/make.inc

LIB_DIR = $(SYS_DIR)/build/$(HW)

TASK = xwm_openlook
BUILD_DIR = $(SW_ROOT_DIR)/build/$(HW)/sbin/x

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(LIB_DIR)/lib/libx++.a \
		$(LIB_DIR)/lib/libx.a \
		$(LIB_DIR)/lib/libfont.a \
		$(LIB_DIR)/lib/libgraph.a \
		$(LIB_DIR)/lib/libcxx.a 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH)  $(EWOK_LIBC)  -lcxx

clean:
	rm -f $(TASK_OBJS)
