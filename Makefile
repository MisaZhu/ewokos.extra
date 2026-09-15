ifeq ($(PORTING),)
export PORTING = ewokos
endif

ifeq ($(OS_TYPE),)
export OS_TYPE = ewokos
endif

DIRS = libs apps x bin

all: $(DIRS)

# apps/x/bin link against libs (SDL2 etc.)
apps x bin: libs
$(DIRS):
	@$(MAKE) -C $@

.PHONY: all clean $(DIRS)

clean:	
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -fr build
