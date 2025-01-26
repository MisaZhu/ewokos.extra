ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

TASK_OBJS = main.o src/engine.o src/camera.o src/clipping.o src/light.o src/matrix.o src/obj.o src/swap.o \
	   src/texture.o src/triangle.o src/vector.o src/display.o

TASK = 3ddemo
TARGET_DIR = $(ROOTFS_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp $(BSP_LFLAGS) -lx -ltinyjson -lsconf -lferox -lopenlibm $(EWOK_LIBC)  -lcxx

install:
	mkdir -p $(TARGET_DIR)
	cp -r $(TASK) res $(TARGET_DIR)
clean:
	rm -f $(TASK_OBJS)
