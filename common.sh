#!/bin/sh

# version
PATCH=0
MINOR=1
MAJOR=0
VERSION="$PATCH.$MINOR.$MAJOR" 

# target
TARGET=libecs
STATIC=$TARGET.$VERSION.a
SHARED=$TARGET.$VERSION.so

# directory structure
SRCDIR=src
INCDIR=include
LIBDIR=lib
OBJDIR=obj
OUTDIR=out

# test location
TESTS=tests

# libraries in use
INCLUDES=
LIBRARIES=

# toolchain flags
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -fPIC $INCLUDES"
LDFLAGS="$LIBRARIES"

# toolchain
AR=ar
CC=gcc
LD=gcc
RANLIB=ranlib
