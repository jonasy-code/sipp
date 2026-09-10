# Makefile for the animation demo.

LIBDIR=../../libsipp

CC = gcc
LIBS = -lpthread
#CC = cc

# Instruction-set flags by host architecture; see ../../libsipp/Makefile.
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
ARCHFLAGS = -msse4.1
else
ARCHFLAGS =
endif

#CFLAGS = -g -I$(LIBDIR)
CFLAGS = -pipe -O3 $(ARCHFLAGS) -I$(LIBDIR) -std=gnu99 -Wall -Wstrict-prototypes -Wold-style-definition

PROGS = animation
OBJS = animation.o scene.o


all:	$(PROGS)

clean:
	rm -f *.o *~ $(PROGS) anim??.p?m anim??.png core


animation: $(OBJS)
	$(CC) -o $@ $(OBJS) -L$(LIBDIR) -lsipp -lm $(LIBS)

animation.o scene.o: scene.h
