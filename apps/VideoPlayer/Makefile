SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

TASK = VideoPlayer
TASK_OBJS = VideoPlayer.o
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/apps/$(TASK)
EXTRA_LIBS_DIR = $(SYS_ROOT_DIR)/build/$(HW)
LIBGCC = $(shell $(CC) -print-libgcc-file-name)

CXXFLAGS += -I $(EXTRA_LIBS_DIR)/include
LDFLAGS += -L $(EXTRA_LIBS_DIR)/lib -lm -lopenlibm

VIDEO_LIBS = $(EXTRA_LIBS_DIR)/lib/libWidgetVideo.a \
	-lavformat -lavcodec -lswscale -lswresample -lavutil

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(EXTRA_LIBS_DIR)/lib/libWidgetVideo.a \
		$(EXTRA_LIBS_DIR)/lib/libavformat.a \
		$(EXTRA_LIBS_DIR)/lib/libavcodec.a \
		$(EXTRA_LIBS_DIR)/lib/libswscale.a \
		$(EXTRA_LIBS_DIR)/lib/libswresample.a \
		$(EXTRA_LIBS_DIR)/lib/libavutil.a
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(VIDEO_LIBS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) $(LIBGCC) -lcxx
	cp -r res $(BUILD_DIR)

clean:
	rm -f $(TASK_OBJS)
