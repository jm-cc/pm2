/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


typedef struct nm_so_heap * nm_so_heap_t;

struct nm_so_heap *
nm_so_heap_create(int nb_entries);

void
nm_so_heap_push(struct nm_so_heap *heap, void *obj);

void *
nm_so_heap_pop(struct nm_so_heap *heap);

int
nm_so_heap_length(struct nm_so_heap *heap);

