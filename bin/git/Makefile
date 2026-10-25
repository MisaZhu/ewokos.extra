SYS_DIR = ../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
UPSTREAM_DIR = upstream
TINYHTTPSC_DIR = ../../libs/ewebview/libtinyhttpsc
UPSTREAM_MAKE = $(MAKE) -C $(UPSTREAM_DIR) SHELL=/bin/sh uname_S=EwokOS \
	ARCH="$(ARCH)" HW="$(HW)" BUILD_DIR="$(abspath $(BUILD_DIR))" \
	CC="$(CC)" AR="$(AR)" \
	CFLAGS="$(CFLAGS) -ffreestanding" \
	LDFLAGS="-nostdlib -Wl,-Ttext=100 -L$(abspath $(BUILD_DIR))/lib" \
	EWOK_LIBC="-Wl,--start-group -lewoksys -lc -lgloss -Wl,--end-group"

.PHONY: all clean upstream-git
all: upstream-git

upstream-git: $(BUILD_DIR)/lib/libtinyhttpsc.a
	$(UPSTREAM_MAKE) git git-remote-http git-remote-https
	mkdir -p $(TARGET_DIR)/bin $(TARGET_DIR)/usr/libexec/git-core
	cp $(UPSTREAM_DIR)/git $(TARGET_DIR)/bin/git
	cp $(UPSTREAM_DIR)/git $(TARGET_DIR)/usr/libexec/git-core/git
	cp $(UPSTREAM_DIR)/git-remote-http $(TARGET_DIR)/usr/libexec/git-core/git-remote-http
	cp $(UPSTREAM_DIR)/git-remote-https $(TARGET_DIR)/usr/libexec/git-core/git-remote-https

$(BUILD_DIR)/lib/libtinyhttpsc.a:
	$(MAKE) -C $(TINYHTTPSC_DIR) OS_TYPE=ewokos ARCH="$(ARCH)" HW="$(HW)"

clean:
	@if [ -f $(UPSTREAM_DIR)/Makefile ]; then $(UPSTREAM_MAKE) clean; fi
	rm -rf build_$(ARCH)
	rm -f $(TARGET_DIR)/bin/git $(TARGET_DIR)/usr/libexec/git-core/git
	rm -f $(TARGET_DIR)/usr/libexec/git-core/git-remote-http
	rm -f $(TARGET_DIR)/usr/libexec/git-core/git-remote-https
