SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

TASK = oggplayer
TASK_OBJS = oggplayer.o
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/bin

CFLAGS += -I $(SW_ROOT_DIR)/libs/libogg/include
CFLAGS += -I $(SW_ROOT_DIR)/libs/libvorbis/include

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -lvorbis -logg -lm $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)