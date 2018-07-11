INSTALL_PATH?=/usr/local

all:
	gcc mws-client.c -o mws-client
	gcc -Wno-deprecated-declarations -Werror $(shell pkg-config --cflags json-c) mws-server.c -ljson-c -o mws-server -L json

install:
	cp mws-client $(INSTALL_PATH)/bin/mws-client
	cp mws-server $(INSTALL_PATH)/bin/mws-server
