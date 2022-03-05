#
# Makefile for the misc demos of SIPP.
#

# These values are used if not overruled from the command line
#CC = cc
CC = gcc -traditional
CFLAGS = -pipe -O2 -I../libsipp
#CFLAGS = -pipe -g -I../libsipp

SHELL = /bin/sh
RM = rm -f

SRCS = torustest.c conetest.c ellipsoid.c prismtest.c chain.c scroll.c\
	teapot.c structure.c planettest.c isy90.c strausstest.c woodtest.c
PROGRAMS = torustest conetest ellipsoid prismtest chain scroll\
	teapot structure planettest isy90 strausstest woodtest


all:
	@echo "If you want to make only the pretty images, type 'make pretty'."
	@echo "If you want to make only the test images, type   'make tests'."
	@echo "If you want to make all images, type             'make images'."
	@echo
	@echo "If you want to make the programs, but not the images,"
	@echo "type 'make programs'."
	@echo


# ================================================================


programs: ../libsipp/libsipp.a $(PROGRAMS)

../libsipp/libsipp.a:
	cd ..; $(MAKE) library


# ================================================================


teapot: teapot.o ../libsipp/libsipp.a
	$(CC) -o teapot teapot.o ../libsipp/libsipp.a -lm $(LIBS)
	
chain: chain.o ../libsipp/libsipp.a
	$(CC) -o chain chain.o ../libsipp/libsipp.a -lm $(LIBS)

structure: structure.o ../libsipp/libsipp.a
	$(CC) -o structure structure.o ../libsipp/libsipp.a -lm $(LIBS)

planettest: planettest.o ../libsipp/libsipp.a
	$(CC) -o planettest planettest.o ../libsipp/libsipp.a -lm $(LIBS)

isy90: isy90.o ../libsipp/libsipp.a
	$(CC) -o isy90 isy90.o ../libsipp/libsipp.a -lm $(LIBS)

ellipsoid:    ellipsoid.o ../libsipp/libsipp.a
	$(CC) -o ellipsoid ellipsoid.o ../libsipp/libsipp.a -lm $(LIBS)

torustest: torustest.o ../libsipp/libsipp.a
	$(CC) -o torustest torustest.o ../libsipp/libsipp.a -lm $(LIBS)

conetest: conetest.o ../libsipp/libsipp.a
	$(CC) -o conetest conetest.o ../libsipp/libsipp.a -lm $(LIBS)

prismtest: prismtest.o ../libsipp/libsipp.a
	$(CC) -o prismtest prismtest.o ../libsipp/libsipp.a -lm $(LIBS)

strausstest: strausstest.o ../libsipp/libsipp.a
	$(CC) -o strausstest strausstest.o ../libsipp/libsipp.a -lm $(LIBS)

woodtest: woodtest.o ../libsipp/libsipp.a
	$(CC) -o woodtest woodtest.o ../libsipp/libsipp.a -lm $(LIBS)

scroll: scroll.o ../libsipp/libsipp.a
	$(CC) -o scroll scroll.o ../libsipp/libsipp.a -lm $(LIBS)


# ================================================================


clean:
	$(RM) *~ .*~ *.o $(PROGRAMS) *.p?m TAGS

tags:
	etags $(SRCS)


# ================================================================

PRETTY = chain.ppm teapot.ppm structure.ppm planet.ppm isy90.ppm scroll.ppm
TESTS = torus.ppm cone.ppm ellipsoid.ppm prism.ppm strauss.ppm wood.ppm
IMAGES = $(PRETTY) $(TESTS)

images: $(IMAGES)
pretty: $(PRETTY)
tests:  $(TESTS)

chain.ppm: chain
	chain
teapot.ppm: teapot
	teapot
structure.ppm: structure
	structure
planet.ppm: planettest
	planettest
isy90.ppm: isy90
	isy90
torus.ppm: torustest
	torustest
cone.ppm: conetest
	conetest
ellipsoid.ppm: ellipsoid
	ellipsoid
prism.ppm: prismtest
	prismtest
strauss.ppm: strausstest
	strausstest
scroll.ppm: scroll
	scroll

timing: $(PROGRAMS)
	for i in $(PROGRAMS); do echo $$i; time $$i; done
