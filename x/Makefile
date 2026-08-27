
DIRS = xwm/openlook xwm/mac1984

all: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all clean $(DIRS)

clean: 
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
