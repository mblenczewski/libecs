# libecs version
PATCH	= 0
MINOR	= 1
MAJOR	= 0
VERSION	= $(MAJOR).$(MINOR).$(PATCH)

# install path
PREFIX	= /usr/local

# libraries in use
INCS	=
LIBS	=

# toolchain flags
CFLAGS	= -std=c11 -Wpedantic $(INCS)
LDFLAGS	= $(LIBS)

# toolchain
AR	= ar
CC	= gcc
LD	= gcc
