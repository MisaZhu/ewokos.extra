
all: 
	mkdir -p build
	cd libs; make
	cd sdl2; make
	cd drivers/netd; make
	cd apps; make
	cd mariox; make
	cd x; make
	cd bin; make

clean:	
	cd libs; make clean
	cd sdl2; make clean
	cd drivers/netd; make clean
	cd apps; make clean
	cd mariox; make clean
	cd x; make clean
	cd bin; make clean
	rm -fr build
