# libecs - Entity Component System library

include config.mk

# target name
TARGET	= libecs
STATIC	= $(TARGET).$(VERSION).a
SHARED	= $(TARGET).$(VERSION).so

# directory structure
SRCDIR	= src
INCDIR	= include
LIBDIR	= lib

OBJDIR	= obj
OUTDIR	= out

TESTS	= tests

# settings sources
_SRC	= ecs.c
SOURCES	= $(patsubst %,$(SRCDIR)/%,$(_SRC))

_INC	= ecs.h ecs_test.h
HEADERS	= $(patsubst %,$(INCDIR)/%,$(_INC)) 

_OBJ	= $(_SRC:%.c=%.o)
OBJECTS	= $(patsubst %,$(OBJDIR)/%,$(_OBJ))

# make targets
all: $(OBJDIR) $(OUTDIR) $(INCDIR)/ecs.h $(OUTDIR)/$(STATIC) $(OUTDIR)/$(SHARED)

clean:
	rm -f $(INCDIR)/ecs.h
	rm -rf $(OUTDIR) $(OBJDIR)
	$(MAKE) -C $(TESTS) clean

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/include/libecs
	cp -rf $(INCDIR)/* $(DESTDIR)$(PREFIX)/include/libecs
	cp out/$(STATIC) $(DESTDIR)$(PREFIX)/lib
	ln -sf $(STATIC) $(DESTDIR)$(PREFIX)/lib/$(TARGET).a
	cp out/$(SHARED) $(DESTDIR)$(PREFIX)/lib
	ln -sf $(SHARED) $(DESTDIR)$(PREFIX)/lib/$(TARGET).so

test:
	$(MAKE) -C $(TESTS)

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/include/libecs
	unlink $(DESTDIR)$(PREFIX)/lib/$(TARGET).a
	rm $(DESTDIR)$(PREFIX)/lib/$(STATIC)
	unlink $(DESTDIR)$(PREFIX)/lib/$(TARGET).so
	rm $(DESTDIR)$(PREFIX)/lib/$(SHARED)

$(INCDIR)/ecs.h:
	cp ecs.h.gen $@
	sed -i "s/@VERSION_PATCH@/$(PATCH)/g" $(INCDIR)/ecs.h
	sed -i "s/@VERSION_MINOR@/$(MINOR)/g" $(INCDIR)/ecs.h
	sed -i "s/@VERSION_MAJOR@/$(MAJOR)/g" $(INCDIR)/ecs.h
	sed -i "s/@VERSION@/\"$(VERSION)\"/g" $(INCDIR)/ecs.h

$(OBJDIR):
	mkdir $(OBJDIR)

$(OUTDIR):
	mkdir $(OUTDIR)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(LD) $(CFLAGS) -Iinclude -c -o $@ $<

$(OUTDIR)/$(STATIC): $(OBJECTS)
	$(AR) crs $@ $^

$(OUTDIR)/$(SHARED): $(OBJECTS)
	$(CC) $(LDFLAGS) -Iinclude -shared -fPIC -o $@ $^

.PHONY: all clean install test uninstall
