#!/bin/sh

# libecs version
LIBECS_PATCH=0
LIBECS_MINOR=1
LIBECS_MAJOR=0
LIBECS_VERSION="$LIBECS_PATCH.$LIBECS_MINOR.$LIBECS_MAJOR" 

# libecs target
TARGET=libecs
STATIC=$TARGET.$LIBECS_VERSION.a
SHARED=$TARGET.$LIBECS_VERSION.so

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
CFLAGS="-std=c11 -Wpedantic -fPIC $INCLUDES"
LDFLAGS="$LIBRARIES"

# toolchain
AR=ar
CC=gcc
LD=gcc
RANLIB=ranlib
