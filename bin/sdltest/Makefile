SW_ROOT_DIR=../..
SYS_DIR=../../../system
include $(SW_ROOT_DIR)/make.inc
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS += -L$(BUILD_DIR)/lib \
	-lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -lvorbis -logg -lz -lopenlibm

CFLAGS += -I $(BUILD_DIR)/include
CXXFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = sdltest.o

TASK = sdltest
TASK_DIR = $(SW_ROOT_DIR)/build/$(HW)/bin

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libSDL2.a \
		$(BUILD_DIR)/lib/libSDL2_image.a \
		$(BUILD_DIR)/lib/libSDL2_mixer.a \
		$(BUILD_DIR)/lib/libSDL2_gfx.a \
		$(BUILD_DIR)/lib/libSDL2_ttf.a \
		$(BUILD_DIR)/lib/libfreetype.a \
		$(BUILD_DIR)/lib/libpng.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	mkdir -p $(TASK_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

clean:
	rm -f $(TASK_OBJS)
	rm -fr $(TASK)

