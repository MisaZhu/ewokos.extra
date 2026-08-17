SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
ROOTFS_APPS = $(BUILD_DIR)/rootfs/apps

# Common flags - removed SDL includes
CFLAGS += -I.. -I $(BUILD_DIR)/include/glcommon -I $(BUILD_DIR)/include/imgui -D__ewokos__
CXXFLAGS += -I.. -I $(BUILD_DIR)/include/glcommon -I $(BUILD_DIR)/include/imgui -D__ewokos__ -fno-threadsafe-statics

# Common LDFLAGS - removed SDL libraries, added xwin libraries
LDFLAGS += -lglcommon -limgui -lportablegl $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

# Source files - removed SDL backend
SRCS = imgui_demo.cpp imgui_main.cpp

# Object files
OBJS = $(patsubst %.cpp,$(ARCH)/%.o,$(SRCS))

# Target
TASK = imgui_demo
TARGET_DIR = $(ROOTFS_APPS)/$(TASK)
TARGET = $(TARGET_DIR)/$(TASK)

.PHONY: all clean

all: $(TARGET)

# Compile imgui_demo.cpp
$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS)   -c $< -o $@

# Link
$(TARGET): $(OBJS) \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewoksys.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libewokstl.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libx.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libfont.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libgraph.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libwidget++.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libglcommon.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libimgui.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libportablegl.a \
		$(SYS_ROOT_DIR)/build/$(HW)/lib/libcxx.a
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(OBJS)  -o $@ $(LDFLAGS)
	cp -r res $(TARGET_DIR)

clean:
	rm -f $(OBJS) 
	rm -f $(TARGET)
