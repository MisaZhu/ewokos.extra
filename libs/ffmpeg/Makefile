BUILD_DIR = ../../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

all: 
	$(MAKE) -C lib -f ewok.mk
	cd bin; make
#	@cp -r data ${TARGET_DIR}

clean:	
	$(MAKE) -C lib -f ewok.mk clean
	cd bin; make clean
