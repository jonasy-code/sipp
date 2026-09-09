# Makefile for the animation demo.

LIBDIR=../../libsipp

CC = gcc
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


all:	$(PROGS)

clean:
	rm -f *.o *~ $(PROGS) anim??.p?m core
 

animation: animation.o
	$(CC) -o $@ animation.o -L$(LIBDIR) -lsipp -lm

