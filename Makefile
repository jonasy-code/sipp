#
# SIPP - a 3-d rendering library
#
# Copyright  Equivalent Software HB
#
# This is the main Makefile for sipp version 3.1. This directory
# contains three subdirectories:
# libsipp: source code for the library itself.
# demo:    source code for demonstrations and tests
# doc:     standard manual pages for the library.
#
# You can either make the library and the tests/demos separately or 
# you can make them all in one make session. 
#
# To create the library, just type 'make library'.
# To make the demo programs, type 'make demos', but if you haven't 
# made the library first, this will be made automatically for you by
# the makefile in the demos directory.
#
# Before you can make anything, though, you may have to edit the 
# following definitions:



# If you don't have alloca(), uncomment the following line:
#ALLOCA = -DHAVE_NO_ALLOCA

# If you have alloca (often in libPW), but don't have alloca.h, uncomment
# the following line:
#ALLOCA = -DHAVE_NO_ALLOCA_H

#
# If you need extra libraries during the link, define them here.
#LIBS=

# LIBDIR is where libsipp.a will be placed when you make install.
# INCLUDEDIR is where the include files will be placed when you make install.
# MANDIR is where the manuals will be placed when you make install.
# MANEXT is the extension the manuals will receive in MANDIR

LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/local/include
MANDIR = /usr/local/man/man3
MANEXT = 3


# The variable EXTRA_FLAGS should be used for any configuration
# flags that you want to use during the compilation.
#
# If you want to make SIPP as a shared library under SunOS you
# need to define the following two targets:
#LIBSH = libsipp.so
#LIBHINST = install.shared
# and the following extra flags if you use gcc:
#EXTRA_FLAGS = -fPIC
# or if you use Sun cc:
#EXTRA_FLAGS = -pic

EXTRA_FLAGS =

# Choose a suitable C compiler and appropriate flags.  SIPP is written
# in C99; a C99 or later compiler is required.
CC = gcc -pipe

# Instruction-set flags, chosen by host architecture.  On x86-64, SSE4.1
# lets the compiler emit floor() as one instruction, which makes the
# procedural-texture shaders 1.7-1.8x faster with identical output; every
# x86-64 CPU since about 2009 has it.  On arm64 (Apple silicon, Linux
# aarch64) the base architecture already provides this, so no flag is
# needed.  -mfma would give a further ~3% on x86-64 but changes rounding.
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
ARCHFLAGS = -msse4.1
else
ARCHFLAGS =
endif

WARNFLAGS = -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes \
            -Wold-style-definition
CFLAGS = -O3 -std=gnu99 $(ARCHFLAGS) $(WARNFLAGS) $(EXTRA_FLAGS)




SHELL = /bin/sh
RM = rm -f


# ================================================================
#           Don't change anything below this line.
# ================================================================

DOCFILES = primitives.man shaders.man sipp.man sipp_pixmap.man geometric.man


MAKEOPTS = CC="$(CC)" \
	CFLAGS="$(ALLOCA) $(CFLAGS) -I../libsipp" \
	LIBS="$(LIBS)"
ANIMMAKEOPTS = CC="$(CC)" \
	CFLAGS="$(ALLOCA) $(CFLAGS) -I../../libsipp" \
	LIBS="$(LIBS)"
        

all: library demos

library:
	cd libsipp; $(MAKE) $(MAKEOPTS) libsipp.a $(LIBSH)

demos:  library
	cd demo; $(MAKE) $(MAKEOPTS) programs
	cd demo/animation; $(MAKE) $(ANIMMAKEOPTS) all


install: library
	cd libsipp; $(MAKE) LIBDIR=$(LIBDIR) INCLUDEDIR=$(INCLUDEDIR) \
                install $(LIBHINST)
	for i in $(DOCFILES) ; do \
	    cp doc/$$i $(MANDIR)/`basename $$i .man`.$(MANEXT); \
	done


clean:
	$(RM) *~ *shar*
	cd libsipp; $(MAKE) clean;
	cd demo; $(MAKE) clean;
	cd demo/animation; $(MAKE) clean;
	cd doc; ls -1 | egrep -v \
        \(\\.man$$\)\|\(\\.tex$$\)\|\(\\.texinfo$$\)\|\(\\.ps$$\) | xargs $(RM)


shar: clean
	shar -a -n sipp-3.0 -L 50 -o sipp-3.0.shar Makefile README CHANGES \
        COMPATIBILITY INSTALL \
	libsipp/* doc/* demo/*
