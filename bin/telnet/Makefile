SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK = telnet 
TASK_OBJS = telnet.o
BUILD_DIR = $(SW_ROOT_DIR)/build/$(HW)/bin

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)