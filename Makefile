all:
	cscope -b
	cd build && cmake ../ && make
	cd hack && make
clean:
	rm -rvf build/* cscope.*
	make -C hack clean
install:all
	- sudo rm /usr/bin/zwm
	sudo cp build/zwm /usr/bin/zwm
	make -C hack install
backup:
	cd .. && tar -c nwm | gzip > nwm.tgz
	cd .. && upld nwm.tgz
restore:clean
	cd .. && dnld nwm.tgz
	cd .. && tar zxvf nwm.tgz
