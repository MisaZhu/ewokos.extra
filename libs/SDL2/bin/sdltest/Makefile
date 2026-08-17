SDL2_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SDL2_ROOT_DIR)/../../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SDL2_ROOT_DIR)/make.inc

TASK = sdltest
TASK_OBJS = $(ARCH)/sdltest.o
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/bin

LDFLAGS += \
	-ljpeg -lgif -lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -ljpeg -lgif -lvorbis -logg -lz -lm -lopenlibm

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewoksys.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewokstl.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libfont.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libgraph.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libwidget++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libSDL2_image.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libSDL2_mixer.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libSDL2_gfx.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libSDL2_ttf.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libSDL2.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libvorbis.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libogg.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libcxx.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx
clean:
	rm -f $(TASK_OBJS)
