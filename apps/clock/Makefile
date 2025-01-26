ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

TASK_OBJS = clock.o

EXTRA_LIBS_DIR = $(ROOT_DIR)/build/$(HW)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include 
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib

TASK = clock
BUILD_DIR = $(ROOT_DIR)/build/$(HW)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)

