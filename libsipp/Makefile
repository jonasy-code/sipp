#
# Makefile for libsipp
#

VERSION = 3.1

# Instruction-set flags, chosen by host architecture.
#
# On x86-64, SSE4.1 lets the compiler emit floor() as one instruction
# instead of a slow bit-manipulation sequence.  noise() calls floor()
# twelve times per evaluation, so the procedural-texture shaders get
# 1.7-1.8x faster, with byte-identical output.  Every x86-64 CPU since
# about 2009 has SSE4.1.  Adding -mfma gives another ~3% but changes
# floating-point rounding (and therefore some pixels), so it is left out.
#
# On arm64 (Apple silicon, Linux aarch64) the equivalent instructions are
# part of the base architecture, so no flag is needed.
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
ARCHFLAGS = -msse4.1
else
ARCHFLAGS =
endif

#These values are default if they are not overridden from the command line:
CC = gcc -pipe
CFLAGS = -O3 $(ARCHFLAGS) -I. -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
#CFLAGS = -g -I.

SHELL = /bin/sh
RM = rm -f

IHDRS = arena.h bezier.h lightsource.h noise.h objects.h rendering.h smalloc.h \
        viewpoint.h pixelbuf.h
EHDRS = sipp.h geometric.h shaders.h primitives.h sipp_pixmap.h sipp_bitmap.h
HDRS = $(IHDRS) $(EHDRS)
SRCS =  rendering.c geometric.c lightsource.c objects.c smalloc.c arena.c \
        transforms.c viewpoint.c pixelbuf.c \
	basic_shader.c mask.c marble.c granite.c noise.c bozo.c bumpy.c \
	planet.c wood.c phong.c strauss.c\
	prism.c cone.c ellipsoid.c torus.c bezier.c bezier_lex.c \
        sipp_pixmap.c sipp_bitmap.c teapot.c
OBJS = rendering.o geometric.o lightsource.o objects.o smalloc.o arena.o \
       transforms.o viewpoint.o basic_shader.o marble.o granite.o mask.o\
       bozo.o bumpy.o planet.o noise.o bezier.o bezier_lex.o pixelbuf.o\
       torus.o cone.o ellipsoid.o prism.o sipp_pixmap.o sipp_bitmap.o\
       wood.o xalloca.o phong.o strauss.o teapot.o


# These are used if no other values are given at the command line:
LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/local/include


all: libsipp.a

libsipp.a:      $(OBJS)
	ar rv $@ $?
	-ranlib $@

patchlevel.h:   Makefile
	rm -f patchlevel.h
	echo \#define VERSION \"$(VERSION)\" > patchlevel.h

libsipp.so:     $(OBJS)
	$(LD) -assert pure-text $(LDFLAGS) -o $(@) $(OBJS)

install: libsipp.a $(EHDRS)
	cp libsipp.a $(LIBDIR)
	cp $(EHDRS) $(INCLUDEDIR)

install.shared: libsipp.so
	cp libsipp.so $(LIBDIR)/libsipp.so.$(VERSION)

tags:
	etags -t $(HDRS) $(SRCS)

clean: 
	$(RM) *.o *~ TAGS libsipp.a
	-$(RM) libsipp.so

rendering.o:         sipp.h rendering.h lightsource.h geometric.h smalloc.h arena.h\
                     objects.h sipp_bitmap.h viewpoint.h patchlevel.h\
                     xalloca.h pixelbuf.h
geometric.o:         geometric.h sipp.h smalloc.h
lightsource.o:       geometric.h lightsource.h sipp.h smalloc.h
objects.o:           sipp.h
pixelbuf.o:          pixelbuf.h sipp.h smalloc.h rendering.h viewpoint.h\
                     lightsource.h
smalloc.o:           
transforms.o:        geometric.h sipp.h
viewpoint.o:         geometric.h smalloc.h viewpoint.h sipp.h
basic_shader.o:      sipp.h geometric.h
strauss.o:           sipp.h shaders.h geometric.h
mask.o:              shaders.h sipp.h
marble.o:            noise.h shaders.h sipp.h geometric.h
granite.o:           noise.h shaders.h sipp.h geometric.h
bozo.o:              noise.h shaders.h sipp.h
bumpy.o:             noise.h shaders.h sipp.h geometric.h
planet.o:            noise.h sipp.h geometric.h
wood.o:              noise.h shaders.h sipp.h
phong.o:             sipp.h shaders.h geometric.h
noise.o:             noise.h sipp.h
bezier.o:            bezier.h sipp.h primitives.h smalloc.h
bezier_lex.o:        bezier.h sipp.h smalloc.h
torus.o:             sipp.h primitives.h xalloca.h
ellipsoid.o:         sipp.h primitives.h xalloca.h
prism.o:             sipp.h primitives.h xalloca.h
cone.o:              sipp.h primitives.h xalloca.h
teapot.o:            sipp.h primitives.h
sipp_pixmap.o:       sipp_pixmap.h sipp.h smalloc.h
sipp_bitmap.o:       sipp_bitmap.h sipp.h smalloc.h
xalloca.o:           xalloca.h
arena.o:             arena.h sipp.h smalloc.h
