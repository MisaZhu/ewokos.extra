SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

TASK = curse_test
TASK_OBJS = $(ARCH)/curse_test.o
BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/rootfs/bin

# curses needs the terminfo back-end and both need libc; list them ahead of the
# grouped core libs so ld resolves left-to-right (curses -> terminfo -> libc).
TASK_LIBS = -lcurses -lterminfo

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libcurses.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libterminfo.a \
		$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)/lib/libewoksys.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(TASK_LIBS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
