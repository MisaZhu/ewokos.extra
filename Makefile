
DIRS = libs apps x bin

all: 
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:	
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -fr build
