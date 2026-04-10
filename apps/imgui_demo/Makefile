SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
ROOTFS_APPS = $(BUILD_DIR)/rootfs/apps

# Common flags - removed SDL includes
CFLAGS += -I.. -I$(SW_ROOT_DIR)/libs/external -DUSING_PORTABLEGL -D__ewokos__
CXXFLAGS += -I.. -I$(SW_ROOT_DIR)/libs/external -DUSING_PORTABLEGL -D__ewokos__ -fno-threadsafe-statics

# Common LDFLAGS - removed SDL libraries, added xwin libraries
LDFLAGS += -lportablegl $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

# glcommon utility sources
GLCOMMON_UTILS = $(SW_ROOT_DIR)/libs/glcommon/src/c_utils.o $(SW_ROOT_DIR)/libs/glcommon/src/gltools.o

# Source files - removed SDL backend
SRCS = imgui_demo.cpp
IMGUI_BACKEND_SRCS = $(SW_ROOT_DIR)/libs/external/imgui/backends/imgui_impl_pgl_geometry.cpp

# Object files
OBJS = $(patsubst %.cpp,%.o,$(SRCS))
IMGUI_BACKEND_OBJS = imgui_impl_pgl_geometry.o

# Target
TASK = imgui_demo
TARGET_DIR = $(ROOTFS_APPS)/$(TASK)
TARGET = $(TARGET_DIR)/$(TASK)

.PHONY: all clean

all: $(TARGET)

# Compile imgui_demo.cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(SW_ROOT_DIR)/libs/portablegl/include -I$(SW_ROOT_DIR)/libs/portablegl/include/portablegl -I$(SW_ROOT_DIR)/libs/glcommon/include -I$(SW_ROOT_DIR)/libs/glcommon/include/glcommon -I$(SW_ROOT_DIR)/libs/external/imgui -I$(SW_ROOT_DIR)/libs/external/imgui/backends -c $< -o $@

# Compile imgui backend files
imgui_impl_pgl_geometry.o: $(SW_ROOT_DIR)/libs/external/imgui/backends/imgui_impl_pgl_geometry.cpp
	$(CXX) $(CXXFLAGS) -I$(SW_ROOT_DIR)/libs/portablegl/include -I$(SW_ROOT_DIR)/libs/portablegl/include/portablegl -I$(SW_ROOT_DIR)/libs/glcommon/include -I$(SW_ROOT_DIR)/libs/glcommon/include/glcommon -I$(SW_ROOT_DIR)/libs/external/imgui -I$(SW_ROOT_DIR)/libs/external/imgui/backends -c $< -o $@

# Compile glcommon utility files
$(SW_ROOT_DIR)/libs/glcommon/src/c_utils.o: $(SW_ROOT_DIR)/libs/glcommon/src/c_utils.c
	$(CC) $(CFLAGS) -I$(SW_ROOT_DIR)/libs/glcommon/include -I$(SW_ROOT_DIR)/libs/glcommon/include/glcommon -c $< -o $@

$(SW_ROOT_DIR)/libs/glcommon/src/gltools.o: $(SW_ROOT_DIR)/libs/glcommon/src/gltools.c
	$(CC) $(CFLAGS) -I$(SW_ROOT_DIR)/libs/glcommon/include -I$(SW_ROOT_DIR)/libs/glcommon/include/glcommon -DNO_TLS -c $< -o $@

# Link
$(TARGET): $(OBJS) $(IMGUI_BACKEND_OBJS) $(GLCOMMON_UTILS)
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(OBJS) $(IMGUI_BACKEND_OBJS) $(GLCOMMON_UTILS) -o $@ $(LDFLAGS)
	cp -r res $(TARGET_DIR)

clean:
	rm -f $(OBJS) $(IMGUI_BACKEND_OBJS)
	rm -f $(TARGET)
