
all: 
	cd SDL2; make
	cd ffmpeg; make
	cd portablegl; make
	cd libs; make
	cd apps; make
	cd x; make
	cd bin; make

clean:	
	cd libs; make clean
	cd SDL2; make clean
	cd ffmpeg; make clean
	cd portablegl; make clean
	cd apps; make clean
	cd x; make clean
	cd bin; make clean
	rm -fr build
