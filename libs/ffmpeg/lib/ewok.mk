FFMPEG_ROOT_DIR = ..
EWOKOS_ROOT_DIR = $(FFMPEG_ROOT_DIR)/../../..
include $(FFMPEG_ROOT_DIR)/make.inc

SYS_BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
PREFIX = $(SYS_BUILD_DIR)
FFMPEG_MAKE = $(MAKE) -f Makefile
CROSS_PREFIX = $(patsubst %gcc,%,$(CC))
FFMPEG_ARCH = $(ARCH)
FFMPEG_EXTRA_CFLAGS =

ifeq ($(ARCH),arm)
ifeq ($(ARCH_VER),v7)
FFMPEG_EXTRA_CFLAGS += -march=armv7-a
endif
endif

ifeq ($(ARCH),x86)
FFMPEG_EXTRA_CONFIGURE_FLAGS += --disable-x86asm
endif

CONFIGURE_FLAGS = \
	--prefix=$(PREFIX) \
	--libdir=$(PREFIX)/lib \
	--incdir=$(PREFIX)/include \
	--pkgconfigdir=$(PREFIX)/lib/pkgconfig \
	--enable-cross-compile \
	--arch=$(FFMPEG_ARCH) \
	--target-os=linux \
	--cross-prefix=$(CROSS_PREFIX) \
	--cc=$(CC) \
	--cxx=$(CXX) \
	--ar=$(AR) \
	--ranlib=$(CROSS_PREFIX)ranlib \
	--nm=$(CROSS_PREFIX)nm \
	--disable-programs \
	--disable-doc \
	--disable-debug \
	--disable-autodetect \
	--disable-network \
	--disable-iconv \
	--disable-zlib \
	--disable-bzlib \
	--disable-lzma \
	--disable-sdl2 \
	--disable-xlib \
	--disable-vulkan \
	--disable-vaapi \
	--disable-vdpau \
	--disable-postproc \
	--disable-avdevice \
	--disable-avfilter \
	--disable-pthreads \
	--disable-w32threads \
	--disable-os2threads \
	--enable-asm \
	--disable-everything \
	--enable-static \
	--disable-shared \
	--enable-small \
	--enable-avcodec \
	--enable-avformat \
	--enable-avutil \
	--enable-swresample \
	--enable-swscale \
	--enable-protocol=file \
	--enable-demuxer=avi,flac,matroska,mov,mp3,ogg,wav \
	--enable-parser=aac,aac_latm,flac,h264,hevc,mpegaudio,mpeg4video,opus,vorbis \
	--enable-decoder=aac,flac,h264,hevc,mjpeg,mp3,mpeg2video,mpeg4,opus,pcm_f32le,pcm_s16be,pcm_s16le,pcm_u8,vorbis \
	--enable-bsf=aac_adtstoasc \
	--extra-cflags="$(CFLAGS) $(FFMPEG_EXTRA_CFLAGS) -isystem $(SYS_BUILD_DIR)/include -include string.h -include math.h" \
	--extra-ldflags="-nostartfiles -nostdlib -L$(SYS_BUILD_DIR)/lib -Wl,-Ttext=100" \
	--extra-libs="-Wl,--start-group -lewoksys -lc -lgloss -lgcc -Wl,--end-group -lm -lopenlibm" \
	$(FFMPEG_EXTRA_CONFIGURE_FLAGS)

TARGET_LIBS = \
	$(SYS_BUILD_DIR)/lib/libavcodec.a \
	$(SYS_BUILD_DIR)/lib/libavformat.a \
	$(SYS_BUILD_DIR)/lib/libavutil.a \
	$(SYS_BUILD_DIR)/lib/libswresample.a \
	$(SYS_BUILD_DIR)/lib/libswscale.a

all: $(TARGET_LIBS)

ffbuild/config.mak: configure
	@test -f libavfilter/allfilters.c || : > libavfilter/allfilters.c
	@test -f libavdevice/alldevices.c || : > libavdevice/alldevices.c
	./configure $(CONFIGURE_FLAGS)

$(TARGET_LIBS): ffbuild/config.mak
	$(FFMPEG_MAKE)
	$(FFMPEG_MAKE) install
	cp libavcodec/libavcodec.a $(SYS_BUILD_DIR)/lib/
	cp libavformat/libavformat.a $(SYS_BUILD_DIR)/lib/
	cp libavutil/libavutil.a $(SYS_BUILD_DIR)/lib/
	cp libswresample/libswresample.a $(SYS_BUILD_DIR)/lib/
	cp libswscale/libswscale.a $(SYS_BUILD_DIR)/lib/

clean:
	-$(FFMPEG_MAKE) clean
	-$(FFMPEG_MAKE) distclean
	rm -f $(TARGET_LIBS)
