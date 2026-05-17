SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LIB_WIDGET_VIDEO_OBJS = src/WidgetVideo/WidgetVideo.o
LIB_WIDGET_VIDEO = $(TARGET_DIR)/lib/libWidgetVideo.a

all: $(LIB_WIDGET_VIDEO)

$(LIB_WIDGET_VIDEO): $(LIB_WIDGET_VIDEO_OBJS)
	@mkdir -p $(TARGET_DIR)/lib
	@mkdir -p $(TARGET_DIR)/include
	$(AR) rcs $(LIB_WIDGET_VIDEO) $(LIB_WIDGET_VIDEO_OBJS)
	cp -r include/* $(TARGET_DIR)/include/

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_WIDGET_VIDEO_OBJS)
	rm -f $(LIB_WIDGET_VIDEO)
