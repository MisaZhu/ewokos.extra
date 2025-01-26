ifeq ($(ARCH),)
export ARCH=arm
endif

SDK_DIR= /usr/local/ewokos/$(ARCH)
ROOTFS_DIR= /usr/local/ewokos/$(ARCH)/rootfs
INST_DIR= /usr/local/ewokos

include $(SDK_DIR)/make.rule

LDFLAGS = -L$(SDK_DIR)/lib
CFLAGS += -I include -I $(SDK_DIR)/include
CXXFLAGS += -I include -I $(SDK_DIR)/include

EWOK_LIBC = -lewoksys -lc -lgloss -lsoftfloat