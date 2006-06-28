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

#ifndef NM_SO_STACK_H
#define NM_SO_STACK_H

typedef struct nm_so_stack * nm_so_stack_t;

struct nm_so_stack *
nm_so_stack_create(int nb_entries);

void
nm_so_stack_free(struct nm_so_stack *stack);


void
nm_so_stack_push(struct nm_so_stack *stack, void *obj);

void *
nm_so_stack_pop(struct nm_so_stack *stack);

int
nm_so_stack_size(struct nm_so_stack *stack);

void *
nm_so_stack_top(struct nm_so_stack *stack);

void *
nm_so_stack_down(struct nm_so_stack *stack);

#endif
