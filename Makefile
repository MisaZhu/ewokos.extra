
all: 
	mkdir -p build
	cd libs; make
	cd drivers/netd; make
	cd apps; make
	cd mario_vm; make
	cd x; make
	cd sdl2; make
	cd bin; make

clean:	
	cd libs; make clean
	cd drivers/netd; make clean
	cd apps; make clean
	cd mario_vm; make clean
	cd x; make clean
	cd sdl2; make clean
	cd bin; make clean
	rm -fr build
