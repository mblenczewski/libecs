### executable name
TARGET	= ecs

### directory structure
SRCDIR	= src
INCDIR	= include
LIBDIR	= lib

BINDIR	= bin

### toolchain settings
FLAGS	= -std=c99 -Wpedantic

CC	= gcc
CCFLAGS	= -I$(INCDIR)

LD	= gcc
LDFLAGS	= -L$(LIBDIR)

### settings sources
vpath %.h $(INCDIR)
vpath %.c $(SRCDIR)

SOURCES	= main.c ecs.c
HEADERS	= ecs.h
LIBS	= 

### make targets
all: $(BINDIR) $(BINDIR)/$(TARGET)

clean:
	-rm -rf $(OBJDIR) $(BINDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/$(TARGET): $(SOURCES)
	$(CC) $(FLAGS) $(CCFLAGS) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: all clean
