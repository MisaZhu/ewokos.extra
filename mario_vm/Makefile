SW_ROOT_DIR=..
SYS_DIR=$(SW_ROOT_DIR)/../system
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SYS_DIR)/build/$(HW)

MARIO_VM= .
include $(MARIO_VM)/lang/js/lang.mk

mario_OBJS = $(MARIO_VM)/mario/mario.o
platform_OBJS = $(MARIO_VM)/platform/platform.o \
		$(MARIO_VM)/platform/mem.o

lib_OBJS = bin/lib/js.o bin/lib/vm_json_var.o bin/lib/native_UniObject.o bin/lib/mbc.o
mvm_OBJS = bin/mario/main.o $(lib_OBJS)
wjsvm_OBJS = bin/wjs/wjs.o bin/wjs/native_wjs.o $(lib_OBJS)
bcasm_OBJS = bin/bcasm/main.o $(MARIO_VM)/mario/bcdump/bcdump.o bin/lib/mbc.o bin/lib/js.o

MARIO_OBJS = $(mario_OBJS) $(mvm_OBJS) $(lang_OBJS) $(platform_OBJS) \
		$(NATIVE_OBJS)

WJS_OBJS = $(mario_OBJS) $(wjsvm_OBJS) $(lang_OBJS) $(platform_OBJS) \
		$(NATIVE_OBJS)

BCASM_OBJS = $(mario_OBJS) $(bcasm_OBJS) $(lang_OBJS) $(platform_OBJS)

ifneq ($(MARIO_DEBUG), no)
CFLAGS += -g -DMARIO_DEBUG
else
CFLAGS += -O2
endif

ifneq ($(MARIO_CACHE), no)
CFLAGS += -DMARIO_CACHE
endif

ifneq ($(MARIO_THREAD), no)
CFLAGS += -DMARIO_THREAD
endif

HEADS = \
	-I$(NATIVE_PATH_BUILTIN) \
	-I$(NATIVE_PATH_GRAPH) \
	-I$(NATIVE_PATH_X) \
	-I$(MARIO_VM)/mario \
	-I$(MARIO_VM)/bin/lib \
	-I$(MARIO_VM)/platform

CFLAGS += $(HEADS)
CXXFLAGS += $(HEADS)

TARGET_DIR = $(SW_ROOT_DIR)/build/$(HW)/bin
MARIO = $(TARGET_DIR)/mario
WJS = $(TARGET_DIR)/wjs
BCASM = $(TARGET_DIR)/bcasm

all: $(MARIO) $(BCASM) $(WJS)
	@echo "done"


$(MARIO): $(MARIO_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libpng.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(MARIO_OBJS) -o $(MARIO) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(WJS): $(WJS_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libpng.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(WJS_OBJS) -o $(WJS) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx


$(BCASM): $(BCASM_OBJS)
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(BCASM_OBJS) -o $(BCASM) $(LDFLAGS) -lbsp $(BSP_LFLAGS)  $(EWOK_LIBC)

clean:
	rm -fr $(MARIO) $(BCASM)
	rm -fr $(MARIO_OBJS) $(BCASM_OBJS)
