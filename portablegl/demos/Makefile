SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
ROOTFS_BIN = $(BUILD_DIR)/rootfs/bin/x

# Common flags
CFLAGS += -I.. -I../external -I$(BUILD_DIR)/include/SDL2 -D__ewokos__
CXXFLAGS += -I.. -I../external -I$(BUILD_DIR)/include/SDL2 -I../glcommon/include -I../glcommon/include/glcommon -D__ewokos__ -fno-threadsafe-statics

# Common LDFLAGS
LDFLAGS += -lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -lvorbis -logg -lz -lm -lopenlibm -lportablegl $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

# glcommon library
GLCOMMON_LIB = $(BUILD_DIR)/lib/libglcommon.a \
	$(BUILD_DIR)/lib/libportablegl.a

# C++ files - all refactored to use portablegl's built-in types with PGL_PREFIX_TYPES
# Now all demos compile with C-style code using pgl_compat.h
CPP_SRCS = shadertoy.cpp sphere.cpp cubemap.cpp grass.cpp

C_SRCS = balls.c

# Object files
C_OBJS = $(patsubst %.c,%.o,$(C_SRCS))
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS))

# Targets in rootfs
C_BINS = $(patsubst %.c,$(ROOTFS_BIN)/%,$(C_SRCS))
CPP_BINS = $(patsubst %.cpp,$(ROOTFS_BIN)/%,$(CPP_SRCS))

ALL_BINS = $(C_BINS) $(CPP_BINS)

.PHONY: all clean dirs

all: dirs $(ALL_BINS)

dirs:
	@mkdir -p $(ROOTFS_BIN)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -I../portablegl/include -I../portablegl/include/portablegl -c $< -o $@

# Compile C++ files (include glcommon for rsw_math and gltools)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I../portablegl/include -I../portablegl/include/portablegl -I../glcommon/include -I../glcommon/include/glcommon -I$(BUILD_DIR)/include -c $< -o $@

# Link C programs
$(ROOTFS_BIN)/%: %.o $(GLCOMMON_LIB) $(BUILD_DIR)/lib/libSDL2.a
	$(LD) -Ttext=100 $< $(GLCOMMON_LIB) -o $@ $(LDFLAGS)

clean:
	rm -f $(C_OBJS) $(CPP_OBJS)
	rm -f $(ALL_BINS)
