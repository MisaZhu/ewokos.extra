
SW_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SW_ROOT_DIR)/..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SW_ROOT_DIR)/make.inc

.DEFAULT_GOAL := all

# LiteHTML source files
LITEHTML_SRCS = \
	libs/litehtml/src/el_break.cpp \
	libs/litehtml/src/el_para.cpp \
	libs/litehtml/src/el_text.cpp \
	libs/litehtml/src/el_script.cpp \
	libs/litehtml/src/el_style.cpp \
	libs/litehtml/src/el_table.cpp \
	libs/litehtml/src/media_query.cpp \
	libs/litehtml/src/document.cpp \
	libs/litehtml/src/element.cpp \
	libs/litehtml/src/el_image.cpp \
	libs/litehtml/src/el_base.cpp \
	libs/litehtml/src/el_anchor.cpp \
	libs/litehtml/src/html_tag.cpp \
	libs/litehtml/src/el_comment.cpp \
	libs/litehtml/src/iterators.cpp \
	libs/litehtml/src/background.cpp \
	libs/litehtml/src/el_font.cpp \
	libs/litehtml/src/html.cpp \
	libs/litehtml/src/css_selector.cpp \
	libs/litehtml/src/el_td.cpp \
	libs/litehtml/src/el_tr.cpp \
	libs/litehtml/src/context.cpp \
	libs/litehtml/src/el_body.cpp \
	libs/litehtml/src/table.cpp \
	libs/litehtml/src/style.cpp \
	libs/litehtml/src/box.cpp \
	libs/litehtml/src/el_div.cpp \
	libs/litehtml/src/el_before_after.cpp \
	libs/litehtml/src/el_link.cpp \
	libs/litehtml/src/css_length.cpp \
	libs/litehtml/src/el_space.cpp \
	libs/litehtml/src/el_cdata.cpp \
	libs/litehtml/src/el_title.cpp \
	libs/litehtml/src/web_color.cpp \
	libs/litehtml/src/stylesheet.cpp

LITEHTML_OBJS = $(patsubst %.cpp,%.o,$(LITEHTML_SRCS))

# Gumbo parser source files
GUMBO_SRCS = \
	libs/litehtml/src/gumbo/vector.c \
	libs/litehtml/src/gumbo/util.c \
	libs/litehtml/src/gumbo/utf8.c \
	libs/litehtml/src/gumbo/tokenizer.c \
	libs/litehtml/src/gumbo/string_piece.c \
	libs/litehtml/src/gumbo/tag.c \
	libs/litehtml/src/gumbo/string_buffer.c \
	libs/litehtml/src/gumbo/parser.c \
	libs/litehtml/src/gumbo/error.c \
	libs/litehtml/src/gumbo/char_ref.c \
	libs/litehtml/src/gumbo/attribute.c

GUMBO_OBJS = $(patsubst %.c,%.o,$(GUMBO_SRCS))

# Application source files
TASK_OBJS = \
		src/CBrowserWnd.o \
		src/SDLContainer.o \
		src/el_input.o \
		src/main.o \
		$(LITEHTML_OBJS) \
		$(GUMBO_OBJS)

CXXFLAGS += -nostdinc++ -I libs/litehtml/include -I $(SDK_DIR)/include/SDL2 -D__EWOKOS__

CPP_OBJS = src/CBrowserWnd.o src/SDLContainer.o src/el_input.o src/main.o $(LITEHTML_OBJS)
$(CPP_OBJS): $(STL_HEADERS_STAMP)

CFLAGS += -I libs/litehtml/include -I libs/litehtml/src/gumbo/include -I $(SDK_DIR)/include/SDL2 -D__EWOKOS__
LDFLAGS += -lSDL2_image -lSDL2_mixer -lSDL2_gfx -lSDL2_ttf -lSDL2 -lfreetype -lpng -lvorbis -logg -lz -lopenlibm

TASK = SDLBrowser
BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)/rootfs/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx
	cp -r res $(BUILD_DIR)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

LITEHTML_CXXFLAGS = $(filter-out -fno-rtti,$(CXXFLAGS)) -nostdinc++ -I libs/litehtml/include -I $(SDK_DIR)/include/SDL2 -D__EWOKOS__

libs/litehtml/src/%.o: libs/litehtml/src/%.cpp
	$(CXX) $(LITEHTML_CXXFLAGS) -c $< -o $@

libs/litehtml/src/gumbo/%.o: libs/litehtml/src/gumbo/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TASK_OBJS) $(CPP_OBJS) $(BUILD_DIR)/$(TASK)
