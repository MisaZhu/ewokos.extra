SW_ROOT_DIR=..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)

all: include/portablegl/portablegl.h
	cp -r include $(BUILD_DIR)
	@echo "all done."