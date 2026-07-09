
all: 
	cd libs; make
	cd apps; make
	cd x; make
	cd bin; make

clean:	
	cd libs; make clean
	cd apps; make clean
	cd x; make clean
	cd bin; make clean
	rm -fr build
