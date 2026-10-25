# EwokOS cross-build configuration. The command-line implementation remains upstream.
prefix = /usr
gitexecdir = /usr/libexec/git-core
template_dir = /usr/share/git-core/templates
ETC_GITCONFIG = /etc/gitconfig
SHELL_PATH = /bin/sh
PAGER_ENV = LESS=FRX LV=-c

NO_OPENSSL = YesPlease
NO_EXPAT = YesPlease
NO_GETTEXT = YesPlease
NO_ICONV = YesPlease
NO_PERL = YesPlease
NO_PYTHON = YesPlease
NO_TCLTK = YesPlease
NO_REGEX = YesPlease
NO_MMAP = YesPlease
NO_PTHREADS = YesPlease
NO_UNIX_SOCKETS = YesPlease
NO_IPV6 = YesPlease
NO_NSEC = YesPlease
NO_SYMLINK_HEAD = YesPlease
NO_MEMMEM = YesPlease
NEEDS_SOCKET = YesPlease
BLK_SHA1 = YesPlease
HAVE_STRINGS_H = YesPlease
NO_INSTALL_HARDLINKS = YesPlease
NO_CROSS_DIRECTORY_HARDLINKS = YesPlease
SKIP_DASHED_BUILT_INS = YesPlease
CURL_CONFIG = /usr/bin/true

PORT_DIR = $(abspath $(CURDIR)/../port/ewokos)
BASIC_CFLAGS += -I$(BUILD_DIR)/include -I$(PORT_DIR) -include $(PORT_DIR)/preinclude.h -D__ewokos__=1 -D_LITTLE_ENDIAN=1
COMPAT_OBJS += $(PORT_DIR)/compat.o $(PORT_DIR)/curl_compat.o
CURL_LDFLAGS =
EXTLIBS += -ltinyhttpsc $(EWOK_LIBC) -lopenlibm -lgcc
