# Makefile for the animation demo.

LIBDIR=../../libsipp

CC = gcc -traditional
#CC = cc
#CFLAGS = -g -I$(LIBDIR)
#CFLAGS = -O4 -I$(LIBDIR)
CFLAGS = -pipe -O2 -I$(LIBDIR)

PROGS = animation


all:	$(PROGS)

clean:
	rm -f *.o *~ $(PROGS) anim??.p?m core
 

animation: animation.o
	$(CC) -o $@ animation.o -L$(LIBDIR) -lsipp -lm

