ROOT_DIR=../..
include $(ROOT_DIR)/make.inc

TASK_OBJS = main.o src/engine.o src/camera.o src/clipping.o src/light.o src/matrix.o src/obj.o src/swap.o \
	   src/texture.o src/triangle.o src/vector.o src/display.o

EXTRA_LIBS_DIR = $(ROOT_DIR)/build/$(HW)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include 
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib

TASK = 3ddemo
BUILD_DIR = $(ROOT_DIR)/build/$(HW)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
