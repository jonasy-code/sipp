/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Equivalent Software HB  1992
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
 ** arena.c - Simple bump ("arena") allocator, see arena.h.
 **/

#include <sipp.h>

#include <arena.h>
#include <smalloc.h>

/*
 * Every allocation is rounded up to a multiple of this, so any object
 * type may be stored in the arena.  16 covers doubles, pointers and
 * SSE-style vector types on all common platforms.
 */
#define ARENA_ALIGN 16
#define ALIGN_UP(n) (((n) + (ARENA_ALIGN - 1)) & ~(size_t)(ARENA_ALIGN - 1))

/*
 * Used if an arena is used before arena_init() (e.g. a zero-initialized
 * static Arena), so that it still behaves sensibly.
 */
#define ARENA_DEFAULT_BLOCK (64 * 1024)

/*
 * Allocate a fresh block able to hold at least SIZE bytes.
 * The block header and the data area live in one malloc()ed chunk.
 */
static Arena_block *block_create(size_t size) {
  Arena_block *block;
  size_t hdr;

  hdr = ALIGN_UP(sizeof(Arena_block));
  block = (Arena_block *)smalloc((int)(hdr + size));
  block->next = NULL;
  block->size = size;
  block->used = 0;
  block->data = (char *)block + hdr;

  return block;
}

/*
 * Initialize an arena.  No memory is allocated until the first
 * arena_alloc().
 */
void arena_init(Arena *arena, size_t block_size) {
  arena->first = NULL;
  arena->curr = NULL;
  arena->block_size = block_size;
}

/*
 * Return SIZE bytes of memory, suitably aligned.  The memory stays
 * valid until the next arena_reset() or arena_release().
 */
void *arena_alloc(Arena *arena, size_t size) {
  Arena_block *block;
  void *p;

  size = ALIGN_UP(size);
  if (arena->block_size == 0) {
    arena->block_size = ARENA_DEFAULT_BLOCK;
  }

  /*
   * Walk forward through the chain until we find a block with
   * room.  After a reset all blocks are empty, so this usually
   * means "stay in the current block" or "step to the next one".
   */
  block = arena->curr;
  while (block != NULL && block->used + size > block->size) {
    block = block->next;
    if (block != NULL) {
      arena->curr = block;
    }
  }

  if (block == NULL) {
    /*
     * No block with room.  Make a new one and link it after the
     * current block (or as the first block).  Oversized requests
     * get a block of their own.
     */
    block = block_create((size > arena->block_size) ? size : arena->block_size);
    if (arena->curr == NULL) {
      arena->first = block;
    } else {
      block->next = arena->curr->next;
      arena->curr->next = block;
    }
    arena->curr = block;
  }

  p = block->data + block->used;
  block->used += size;

  return p;
}

/*
 * Forget everything allocated so far, but keep the blocks for reuse.
 */
void arena_reset(Arena *arena) {
  Arena_block *block;

  for (block = arena->first; block != NULL; block = block->next) {
    block->used = 0;
  }
  arena->curr = arena->first;
}

/*
 * Free all memory held by the arena.
 */
void arena_release(Arena *arena) {
  Arena_block *block, *next;

  for (block = arena->first; block != NULL; block = next) {
    next = block->next;
    sfree(block);
  }
  arena->first = NULL;
  arena->curr = NULL;
}
