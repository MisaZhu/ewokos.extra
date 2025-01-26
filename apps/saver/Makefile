ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

EXTRA_LIBS_DIR = ../../libs

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/ferox/include 
LDFLAGS += -L $(EXTRA_LIBS_DIR)/ferox

TASK_OBJS = main.o

TASK = saver
BUILD_DIR = $(ROOT_DIR)/build/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp $(BSP_LFLAGS) -lx -ltinyjson -lsconf -lferox -lopenlibm $(EWOK_LIBC)  -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
