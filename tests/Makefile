# libecs tests

include ../config.mk

SOURCES	= test_mem.c
TESTS	= $(SOURCES:%.c=%)

all: $(TESTS)

clean:
	rm -f $(TESTS)

run: $(TESTS)
	for test in $(TESTS); do ./$$test; done

$(TESTS): %: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I../include -o $@ $^

.PHONY: all clean run
