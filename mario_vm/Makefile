ROOT_DIR=..
include $(ROOT_DIR)/make.inc

MARIO_VM= .
include $(MARIO_VM)/lang/js/lang.mk

mario_OBJS = $(MARIO_VM)/mario/mario.o
platform_OBJS = $(MARIO_VM)/platform/platform.o \
		$(MARIO_VM)/platform/mem.o
mvm_OBJS = bin/mario/main.o bin/lib/mbc.o bin/lib/js.o 
bcasm_OBJS = bin/bcasm/main.o bin/bcasm/dump.o bin/lib/mbc.o bin/lib/js.o 

MARIO_OBJS = $(mario_OBJS) $(mvm_OBJS) $(lang_OBJS) $(platform_OBJS) \
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

BUILD_DIR = $(ROOT_DIR)/build/bin
MARIO = $(BUILD_DIR)/mario
BCASM = $(BUILD_DIR)/bcasm

all: $(MARIO) $(BCASM)
	@echo "done"


$(MARIO): $(MARIO_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(MARIO_OBJS) -o $(MARIO) $(LDFLAGS) -lttf -lfont  -lgraph -lbsp $(BSP_LFLAGS) -lupng -lx++ -lx -ltinyjson -lsconf  $(EWOK_LIBC) -lcxx

$(BCASM): $(BCASM_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(BCASM_OBJS) -o $(BCASM) $(LDFLAGS) -lbsp $(BSP_LFLAGS)  $(EWOK_LIBC)

clean:
	rm -fr $(MARIO_OBJS)
