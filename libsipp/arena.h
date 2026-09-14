/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright 1992 Jonas Yngvesson, Inge Wallin
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

/**
 ** arena.h - Simple bump ("arena") allocator for short-lived objects.
 **
 ** An arena hands out memory by bumping a pointer inside a large block,
 ** and allocates a new block when the current one is full.  Individual
 ** objects are never freed; instead the whole arena is reset in one go,
 ** which just rewinds to the first block and keeps the blocks around for
 ** reuse.  This is a good fit for the renderer, where all View_coords for
 ** a polygon die together and all Edges for a rendering pass die together.
 **
 ** Blocks are never moved or reallocated, so pointers handed out by
 ** arena_alloc() stay valid until the next arena_reset()/arena_release().
 **/

#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct arena_block_t {
  struct arena_block_t *next;
  size_t size; /* Usable bytes in this block */
  size_t used; /* Bytes handed out so far */
  char *data;  /* Aligned start of usable memory */
} Arena_block;

typedef struct {
  Arena_block *first; /* Head of the block chain */
  Arena_block *curr;  /* Block currently being bumped */
  size_t block_size;  /* Default size of new blocks */
} Arena;

EXTERN void arena_init(Arena *arena, size_t block_size);

EXTERN void *arena_alloc(Arena *arena, size_t size);

EXTERN void arena_reset(Arena *arena);

EXTERN void arena_release(Arena *arena);

#endif /* ARENA_H */
