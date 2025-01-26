ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

TASK_OBJS = main.o \
		src/InfoNES.o \
		src/InfoNES_Mapper.o \
		src/InfoNES_pAPU.o  \
		src/K6502.o

TASK = emu
TARGET_DIR = $(ROOTFS_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp $(BSP_LFLAGS) -lx -ltinyjson -lsconf -lopenlibm $(EWOK_LIBC)  -lcxx

install:
	mkdir -p $(TARGET_DIR)
	cp -r $(TASK) res $(TARGET_DIR)

clean:
	rm -f $(TASK_OBJS)
	rm -f $(TASK)
