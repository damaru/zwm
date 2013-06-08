x11-window-id.so: x11-window-id.c
	$(CC) -Os -shared -o $@ $< `pkg-config --cflags --libs x11`
clean:
	rm x11-window-id.so
install:x11-window-id.so
	- sudo rm /usr/lib/libxwmhacks.so
	sudo cp x11-window-id.so /usr/lib/libxwmhacks.so

