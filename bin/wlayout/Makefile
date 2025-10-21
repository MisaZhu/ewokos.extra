SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

TASK_OBJS = wlayout.o

EXTRA_LIBS_DIR = $(SW_ROOT_DIR)/build/$(HW)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include 
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib

TASK = wlayout
BUILD_DIR = $(SW_ROOT_DIR)/build/$(HW)/bin

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx

clean:
	rm -f $(TASK_OBJS)


