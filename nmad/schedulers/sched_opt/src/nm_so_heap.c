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


#include <assert.h>
#include <tbx.h>

struct nm_so_heap{
    int nb_entries;
    void **obj;
    int next_to_add;
};

struct nm_so_heap *
nm_so_heap_create(int nb_entries){
    struct nm_so_heap * heap
        = TBX_MALLOC(sizeof(struct nm_so_heap));
    heap->nb_entries = nb_entries;
    heap->obj = TBX_MALLOC(nb_entries * sizeof(void *));
    heap->next_to_add = 0;
    return heap;
}

void
nm_so_heap_push(struct nm_so_heap *heap, void *obj){
    assert(heap->next_to_add < heap->nb_entries);
    heap->obj[heap->next_to_add] = obj;
    heap->next_to_add++;
}

void *
nm_so_heap_pop(struct nm_so_heap *heap){
    assert(heap->next_to_add > 0);
    void * obj = heap->obj[heap->next_to_add - 1];
    heap->next_to_add--;
    return obj;
}

int
nm_so_heap_length(struct nm_so_heap *heap){
    return heap->next_to_add;
}

