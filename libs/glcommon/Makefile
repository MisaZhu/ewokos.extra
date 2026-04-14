SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)

CFLAGS += -I. -Iinclude -Iinclude/glcommon -I $(BUILD_DIR)/include -I../external -I../portablegl/include -I../portablegl/include/portablegl
CXXFLAGS += -I. -Iinclude -Iinclude/glcommon -I $(BUILD_DIR)/include -I../external -I../portablegl/include -I../portablegl/include/portablegl

# C source files
LIB_GLCOMMON_C_OBJS = \
	src/c_utils.o \
	src/cprimitives.o \
	src/chalfedge.o \
	src/gltools.o \
	src/stb_vorbis.o

# C++ source files
LIB_GLCOMMON_CPP_OBJS = \
	src/Frame.o

LIB_GLCOMMON_OBJS = $(LIB_GLCOMMON_C_OBJS) $(LIB_GLCOMMON_CPP_OBJS)

GLCOMMON = $(BUILD_DIR)/lib/libglcommon.a

$(GLCOMMON): $(LIB_GLCOMMON_OBJS)
	$(AR) rcs $(GLCOMMON) $(LIB_GLCOMMON_OBJS)
	cp -r include/* $(BUILD_DIR)/include

# Compile C files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_GLCOMMON_OBJS)
	rm -f $(GLCOMMON)
