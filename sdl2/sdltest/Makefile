ARCH=arm
SDL2_DIR=../SDL2

SW_ROOT_DIR=../..
SYS_DIR=../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -L$(SDL2_DIR) -lSDL2 -lopenlibm

CFLAGS += -I $(BUILD_DIR)/include -I$(SDL2_DIR)/include
CXXFLAGS += -I $(BUILD_DIR)/include

SDLTEST_OBJS = sdltest.o

SDLTEST = $(SW_ROOT_DIR)/build/$(HW)/bin/sdltest

all: $(SDLTEST)

$(SDLTEST): $(SDLTEST_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(SDLTEST_OBJS) -o $(SDLTEST) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(SDLTEST_DIR):
	mkdir -p $(SDLTEST_DIR)
	cp -r res $(SDLTEST_DIR)

clean:
	rm -f $(SDLTEST_OBJS)
	rm -fr $(SDLTEST_DIR)

