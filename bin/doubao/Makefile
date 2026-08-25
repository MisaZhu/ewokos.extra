SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK = doubao
TASK_OBJS = $(ARCH)/doubao.o
BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/rootfs/bin

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libewoksys.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libtinyhttpsc.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libsocket.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -ltinyhttpsc -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
