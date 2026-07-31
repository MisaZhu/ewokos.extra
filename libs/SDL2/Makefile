BUILD_DIR = ../../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

all: 
	cd libs; $(MAKE) HW=$(HW) ARCH=$(ARCH)
	cd bin; $(MAKE) HW=$(HW) ARCH=$(ARCH)
	cd apps; $(MAKE) HW=$(HW) ARCH=$(ARCH)
#	@cp -r data ${TARGET_DIR}

clean:	
	cd libs; $(MAKE) HW=$(HW) ARCH=$(ARCH) clean
	cd bin; $(MAKE) HW=$(HW) ARCH=$(ARCH) clean
	cd apps; $(MAKE) HW=$(HW) ARCH=$(ARCH) clean
