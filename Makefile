all:
	cscope -b
	cd build && cmake ../ && make
clean:
	rm -rvf build/* cscope.*
install:all
	sudo rm /usr/bin/zwm
	sudo cp build/zwm /usr/bin/zwm
