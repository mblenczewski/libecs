all: build test

build:
	./compile

clean:
	./clean

install: build test
	./install

install-deps:
	./install-deps

run: build test shaders
	./run

shaders:
	./shaders

test: build
	./test

uninstall:
	./uninstall

.PHONY: all build clean install install-deps run shaders test uninstall
