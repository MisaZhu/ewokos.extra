SW_ROOT_DIR=..
include $(SW_ROOT_DIR)/make.inc
SDL2_HEADS_DIR=$(SYS_ROOT_DIR)/build/include/SDL2
SDL2_IMG_HEADS_DIR=$(SYS_ROOT_DIR)/build/include/SDL2
SDL2_MIXER_HEADS_DIR=$(SYS_ROOT_DIR)/build/include/SDL2
SDL2_LIB_DIR=$(SYS_ROOT_DIR)/build/lib

all:
	make -C SDL2
	make -C SDL2_image
	make -C SDL2_mixer
	mkdir -p $(SDL2_HEADS_DIR)
	mkdir -p $(SDL2_IMG_HEADS_DIR)
	mkdir -p $(SDL2_MIXER_HEADS_DIR)
	mkdir -p $(SDL2_LIB_DIR)
	cp SDL2/include/*.h $(SDL2_HEADS_DIR)
	cp SDL2_image/*.h $(SDL2_IMG_HEADS_DIR)
	cp SDL2_mixer/*.h $(SDL2_MIXER_HEADS_DIR)
	cp SDL2/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_image/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_mixer/lib*.a $(SDL2_LIB_DIR)

clean:
	make -C SDL2 -k clean
	make -C SDL2_image -k clean
	make -C SDL2_mixer -k clean
