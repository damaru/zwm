all:
	cd build && cmake ../ && make
clean:
	rm -rvf build/*
install:all
	sudo rm /usr/bin/zwm
	sudo cp build/zwm /usr/bin/zwm
