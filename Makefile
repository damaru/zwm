all: build
	cscope -b
	cd build && cmake ../ && make
	cd hack && make
build:
	mkdir build

clean:
	rm -rvf build/* cscope.*
	make -C hack clean

distclean:
	- find . -name *.un~ | xargs rm

install:all
	- sudo rm /usr/bin/zwm
	sudo cp build/zwm /usr/bin/zwm
	make -C hack install

backup:
	cd .. && tar -c zwm | gzip > zwm.tgz

dep-install:
	sudo apt-get install -y build-essential libx11-dev libxft-dev libxext-dev libxinerama-dev

