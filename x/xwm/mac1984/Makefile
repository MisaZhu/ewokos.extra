TASK_OBJS = xwm.o MacWM.o

SW_ROOT_DIR=../../..
include $(SW_ROOT_DIR)/make.inc

TASK = xwm_mac1984
BUILD_DIR = $(SW_ROOT_DIR)/build/$(HW)/sbin/x

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC)  -lcxx

clean:
	rm -f $(TASK_OBJS)
