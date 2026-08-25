EWOKOS_ROOT_DIR = ../../../..
include ../make.inc

SYS_BUILD_DIR=$(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)
SDL2_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_IMG_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_MIXER_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_GFX_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_TTF_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_LIB_DIR=$(SYS_BUILD_DIR)/lib

all:
	$(MAKE) -C SDL2 HW=$(HW) ARCH=$(ARCH)
	$(MAKE) -C SDL2_image HW=$(HW) ARCH=$(ARCH)
	$(MAKE) -C SDL2_mixer HW=$(HW) ARCH=$(ARCH)
	$(MAKE) -C SDL2_gfx HW=$(HW) ARCH=$(ARCH)
	$(MAKE) -C SDL2_ttf HW=$(HW) ARCH=$(ARCH)
	mkdir -p $(SDL2_HEADS_DIR)
	mkdir -p $(SDL2_IMG_HEADS_DIR)
	mkdir -p $(SDL2_MIXER_HEADS_DIR)
	mkdir -p $(SDL2_GFX_HEADS_DIR)
	mkdir -p $(SDL2_TTF_HEADS_DIR)
	mkdir -p $(SDL2_LIB_DIR)
	cp SDL2/include/*.h $(SDL2_HEADS_DIR)
	cp SDL2_image/*.h $(SDL2_IMG_HEADS_DIR)
	cp SDL2_mixer/*.h $(SDL2_MIXER_HEADS_DIR)
	cp SDL2_gfx/*.h $(SDL2_GFX_HEADS_DIR)
	cp SDL2_ttf/*.h $(SDL2_GFX_HEADS_DIR)
	cp SDL2/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_image/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_mixer/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_gfx/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_ttf/$(ARCH)/lib*.a $(SDL2_LIB_DIR)

clean:
	$(MAKE) -C SDL2 HW=$(HW) ARCH=$(ARCH) -k clean
	$(MAKE) -C SDL2_image HW=$(HW) ARCH=$(ARCH) -k clean
	$(MAKE) -C SDL2_mixer HW=$(HW) ARCH=$(ARCH) -k clean
	$(MAKE) -C SDL2_gfx HW=$(HW) ARCH=$(ARCH) -k clean
	$(MAKE) -C SDL2_ttf HW=$(HW) ARCH=$(ARCH) -k clean
