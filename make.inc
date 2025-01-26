ifeq ($(ARCH),)
export ARCH=arm
endif

INST_DIR= /usr/local/ewokos
SDK_DIR= /usr/local/ewokos/$(ARCH)/$(HW)

include $(INST_DIR)/$(ARCH)/make.rule

LDFLAGS = -L$(SDK_DIR)/lib
CFLAGS += -I include -I $(SDK_DIR)/include
CXXFLAGS += -I include -I $(SDK_DIR)/include

EWOK_LIBC = -lewoksys -lc -lgloss -lsoftfloat
