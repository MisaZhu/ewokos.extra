
all: 
	cd libs; make
	cd apps; make
	cd mario_vm; make

install: 
	cd libs; make install
	cd apps; make install
	cd mario_vm; make install

clean:	
	cd libs; make clean
	cd apps; make clean
	cd mario_vm; make clean
