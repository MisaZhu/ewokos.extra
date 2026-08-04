SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK = doubao
TASK_OBJS = $(ARCH)/doubao.o
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/bin

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -ltinyhttpsc -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
