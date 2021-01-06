all: build test

build:
	./compile

clean:
	./clean

install: build
	./install

test:
	./test

uninstall:
	./uninstall

.PHONY: all build clean install test uninstall
