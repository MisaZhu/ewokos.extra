SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK = vim
TASK_OBJS = $(ARCH)/vim_util.o \
	$(ARCH)/vim_term.o \
	$(ARCH)/vim_input.o \
	$(ARCH)/vim_text.o \
	$(ARCH)/vim_cmd.o \
	$(ARCH)/vim_dcmd.o \
	$(ARCH)/vim_main.o \
	$(ARCH)/vim_syntax.o
BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/rootfs/bin

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libewoksys.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
