
all: 
	mkdir -p build
	cd sdl2; make
	cd portablegl; make
	cd apps; make
	cd x; make
	cd bin; make

clean:	
	cd sdl2; make clean
	cd portablegl; make clean
	cd apps; make clean
	cd x; make clean
	cd bin; make clean
	rm -fr build
