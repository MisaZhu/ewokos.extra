
SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

.DEFAULT_GOAL := all

# Application source files
TASK_OBJS = \
		src/CBrowserWnd.o \
		src/SDLContainer.o \
		src/el_input.o \
		src/main.o

CXXFLAGS += -nostdinc++ -I libs/litehtml/include -I $(SDK_DIR)/include/SDL2 -D__EWOKOS__

CPP_OBJS = src/CBrowserWnd.o src/SDLContainer.o src/el_input.o src/main.o 

CFLAGS += -I $(SDK_DIR)/include/SDL2 -D__EWOKOS__
LDFLAGS += -llitehtml -lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -lvorbis -logg -lz -lopenlibm

TASK = SDLBrowser
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TASK_OBJS) $(CPP_OBJS) $(BUILD_DIR)/$(TASK)
