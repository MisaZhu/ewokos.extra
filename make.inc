ifeq ($(ARCH),)
export ARCH=arm
endif

ifeq ($(HW),)
export HW=versatilepb
endif

EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system

#SDK_DIR= /usr/local/ewokos/$(ARCH)
#ROOTFS_DIR= /usr/local/ewokos/$(ARCH)/$(HW)

SDK_DIR= $(SYS_ROOT_DIR)/build/$(HW)

include $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule

LDFLAGS = -L$(SDK_DIR)/lib
CFLAGS += -I include -I $(SDK_DIR)/include
CXXFLAGS += -I include -I $(SDK_DIR)/include
