SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)
FFMPEG_DIR = $(SW_ROOT_DIR)/ffmpeg
FFMPEG_HEADER = $(TARGET_DIR)/include/libavcodec/avcodec.h
FFMPEG_LIBS = \
	$(TARGET_DIR)/lib/libavcodec.a \
	$(TARGET_DIR)/lib/libavformat.a \
	$(TARGET_DIR)/lib/libavutil.a \
	$(TARGET_DIR)/lib/libswresample.a \
	$(TARGET_DIR)/lib/libswscale.a

LIB_WIDGET_VIDEO_OBJS = src/WidgetVideo/WidgetVideo.o
LIB_WIDGET_VIDEO = $(TARGET_DIR)/lib/libWidgetVideo.a


LIB_WIDGET_WEBVIEW_OBJS = src/WidgetWebview/el_input.o \
	src/WidgetWebview/XContainer.o \
	src/WidgetWebview/WidgetWebview.o
LIB_WIDGET_WEBVIEW = $(TARGET_DIR)/lib/libWidgetWebview.a

# Some targets already have ffmpeg static libraries in the SDK lib dir but are
# missing the installed public headers. Keep the SDK include first, then fall
# back to the local ffmpeg source tree so standalone widget++ builds still work.
CXXFLAGS += -I $(FFMPEG_DIR)/lib

all: $(LIB_WIDGET_VIDEO) $(LIB_WIDGET_WEBVIEW)

$(FFMPEG_HEADER) $(FFMPEG_LIBS):
	$(MAKE) -C $(FFMPEG_DIR)

$(LIB_WIDGET_VIDEO_OBJS): $(FFMPEG_HEADER)

$(LIB_WIDGET_VIDEO): $(FFMPEG_LIBS) $(LIB_WIDGET_VIDEO_OBJS)
	@mkdir -p $(TARGET_DIR)/lib
	@mkdir -p $(TARGET_DIR)/include
	$(AR) rcs $(LIB_WIDGET_VIDEO) $(LIB_WIDGET_VIDEO_OBJS)
	cp -r include/* $(TARGET_DIR)/include/

$(LIB_WIDGET_WEBVIEW): $(LIB_WIDGET_WEBVIEW_OBJS)
	@mkdir -p $(TARGET_DIR)/lib
	@mkdir -p $(TARGET_DIR)/include
	$(AR) rcs $(LIB_WIDGET_WEBVIEW) $(LIB_WIDGET_WEBVIEW_OBJS)
	cp -r include/* $(TARGET_DIR)/include/


%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_WIDGET_VIDEO_OBJS)
	rm -f $(LIB_WIDGET_VIDEO)
	rm -f $(LIB_WIDGET_WEBVIEW_OBJS)
	rm -f $(LIB_WIDGET_WEBVIEW)
