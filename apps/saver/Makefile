ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

TASK_OBJS = main.o

TASK = saver
TARGET_DIR = $(ROOTFS_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp $(BSP_LFLAGS) -lx -ltinyjson -lsconf -lferox -lopenlibm $(EWOK_LIBC)  -lcxx

install:
	mkdir -p $(TARGET_DIR)
	cp -r $(TASK) res $(TARGET_DIR)

clean:
	rm -f $(TASK_OBJS)
	rm -f $(TASK)
