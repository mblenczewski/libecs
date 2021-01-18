#!/bin/sh

# installation prefix
PREFIX=/usr/local

# version
PATCH=0
MINOR=1
MAJOR=0
VERSION="$PATCH.$MINOR.$MAJOR" 

# target
TARGET=libecs
STATIC=$TARGET.a.$VERSION
SHARED=$TARGET.so.$VERSION

# directory structure
SRCDIR=src
INCDIR=include
DEPDIR=deps
OBJDIR=obj
OUTDIR=out

# test location
TESTS=tests

# libraries in use
CGLM="$DEPDIR/cglm-0.7.9"
GLFW="$DEPDIR/glfw-3.3.2"
VK="$DEPDIR/vulkan-1.2.162.1"

INCLUDES="-I$CGLM/include -I$GLFW/include -I$VK/include"
LIBRARIES="-L$CGLM -lcglm -L$GLFW -lglfw -L$VK -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi"

# toolchain flags
NDEBUG="-DNDEBUG"
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -fPIC $INCLUDES $NDEBUG"
LDFLAGS="$LIBRARIES"

# toolchain
AR=ar
CC=gcc
LD=gcc
RANLIB=ranlib
