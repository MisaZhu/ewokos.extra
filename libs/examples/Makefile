SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
ROOTFS_BIN = $(BUILD_DIR)/rootfs/bin/x

# Common flags
CFLAGS += -I.. -I../external -I$(BUILD_DIR)/include/SDL2 -DUSING_PORTABLEGL -D__ewokos__
CXXFLAGS += -I.. -I../external -I$(BUILD_DIR)/include/SDL2 -DUSING_PORTABLEGL -D__ewokos__

# Common LDFLAGS (using portablegl, not glcommon to avoid symbol conflicts)
LDFLAGS += -lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -lvorbis -logg -lz -lm -lopenlibm -lportablegl $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

# glcommon utility sources needed by webgl_lessons
GLCOMMON_UTILS = ../glcommon/src/c_utils.o ../glcommon/src/gltools.o

# Source files
ORIGINAL_C_SRCS = $(wildcard original/*.c)
ORIGINAL_CPP_SRCS = $(wildcard original/*.cpp)
CLASSIC_C_SRCS = $(wildcard classic/*.c)
# webgl_lessons require GLM which is not available in the project
# WEBGL_CPP_SRCS = webgl_lessons/lesson1.cpp ...
WEBGL_CPP_SRCS =

# Object files
ORIGINAL_C_OBJS = $(patsubst original/%.c,original/%.o,$(ORIGINAL_C_SRCS))
ORIGINAL_CPP_OBJS = $(patsubst original/%.cpp,original/%.o,$(ORIGINAL_CPP_SRCS))
CLASSIC_C_OBJS = $(patsubst classic/%.c,classic/%.o,$(CLASSIC_C_SRCS))
WEBGL_CPP_OBJS = $(patsubst webgl_lessons/%.cpp,webgl_lessons/%.o,$(WEBGL_CPP_SRCS))

# Targets in rootfs
ORIGINAL_C_BINS = $(patsubst original/%.c,$(ROOTFS_BIN)/%,$(ORIGINAL_C_SRCS))
ORIGINAL_CPP_BINS = $(patsubst original/%.cpp,$(ROOTFS_BIN)/%,$(ORIGINAL_CPP_SRCS))
CLASSIC_C_BINS = $(patsubst classic/%.c,$(ROOTFS_BIN)/%,$(CLASSIC_C_SRCS))
WEBGL_CPP_BINS = $(patsubst webgl_lessons/%.cpp,$(ROOTFS_BIN)/%,$(WEBGL_CPP_SRCS))

ALL_BINS = $(ORIGINAL_C_BINS) $(ORIGINAL_CPP_BINS) $(CLASSIC_C_BINS) $(WEBGL_CPP_BINS)

.PHONY: all clean dirs

all: dirs $(ALL_BINS)

dirs:
	@mkdir -p $(ROOTFS_BIN)

# Compile glcommon utility files
../glcommon/src/c_utils.o: ../glcommon/src/c_utils.c
	$(CC) $(CFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -c $< -o $@

# Compile gltools without TLS (for embedded systems)
../glcommon/src/gltools.o: ../glcommon/src/gltools.c
	$(CC) $(CFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -DNO_TLS -c $< -o $@

# Compile C files in original (use glcommon)
original/%.o: original/%.c
	$(CC) $(CFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -I../portablegl/include -I../portablegl/include/portablegl -c $< -o $@

# Compile C++ files in original (use glcommon)
original/%.o: original/%.cpp
	$(CXX) $(CXXFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -I../portablegl/include -I../portablegl/include/portablegl -c $< -o $@

# Compile C files in classic (use glcommon)
classic/%.o: classic/%.c
	$(CC) $(CFLAGS) -I../glcommon/include -I../glcommon/include/glcommon -I../portablegl/include -I../portablegl/include/portablegl -c $< -o $@

# Compile C++ files in webgl_lessons (use GLM from glcommon, exclude portablegl types)
webgl_lessons/%.o: webgl_lessons/%.cpp
	$(CXX) $(CXXFLAGS) -I../portablegl/include -I../portablegl/include/portablegl -I../external -I../glcommon/include -I../glcommon/include/glcommon -c $< -o $@

# Link original C programs
$(ROOTFS_BIN)/%: original/%.o
	$(LD) -Ttext=100 $< -o $@ $(LDFLAGS)

# Link original C++ programs
$(ROOTFS_BIN)/%: original/%.o
	$(LD) -Ttext=100 $< -o $@ $(LDFLAGS)

# Link classic C programs
$(ROOTFS_BIN)/%: classic/%.o
	$(LD) -Ttext=100 $< -o $@ $(LDFLAGS)

# Link webgl_lessons C++ programs (with glcommon utils)
$(ROOTFS_BIN)/%: webgl_lessons/%.o $(GLCOMMON_UTILS)
	$(LD) -Ttext=100 $< $(GLCOMMON_UTILS) -o $@ $(LDFLAGS)

clean:
	rm -f $(ORIGINAL_C_OBJS) $(ORIGINAL_CPP_OBJS) $(CLASSIC_C_OBJS) $(WEBGL_CPP_OBJS)
	rm -f $(GLCOMMON_UTILS)
	rm -f $(ALL_BINS)
