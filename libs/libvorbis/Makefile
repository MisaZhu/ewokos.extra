SW_ROOT_DIR=../..
SYS_DIR=../../../system
include $(SW_ROOT_DIR)/make.inc
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps

ASFLAGS = --warn 
CFLAGS += -I $(TARGET_DIR)/include  -DHAVE_CONFIG_H -I. -Iinclude -Ilib -O20 -Wall -Wextra -ffast-math -ffreestanding -D_REENTRANT -fsigned-char -Wdeclaration-after-statement 
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBVORBIS = $(TARGET_DIR)/lib/libvorbis.a
all: $(LIBVORBIS)

OBJS = lib/mdct.o lib/smallft.o lib/block.o lib/envelope.o lib/window.o lib/lsp.o lib/lpc.o lib/analysis.o lib/synthesis.o \
       lib/psy.o lib/info.o lib/floor1.o lib/floor0.o lib/res0.o lib/mapping0.o lib/registry.o lib/codebook.o lib/sharedbook.o \
       lib/lookup.o lib/bitrate.o lib/vorbisfile.o


$(LIBVORBIS): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	cp -r include/* $(TARGET_DIR)/include

%.o: %.c
	@mkdir -p $(DEPDIR)/$(@D)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.cpp
	@mkdir -p $(DEPDIR)/$(@D)
	$(CXX) $(CPPFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) libvorbis.a
	rm -rf $(DEPDIR)
