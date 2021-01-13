all: build test

build:
	./compile

clean:
	./clean

install: build
	./install

run: build test
	./run

test: build
	./test

uninstall:
	./uninstall

.PHONY: all build clean install run test uninstall
