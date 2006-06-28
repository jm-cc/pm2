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

struct nm_so_stack{
    int nb_entries;
    void **obj;
    int next_to_add;
};

struct nm_so_stack *
nm_so_stack_create(int nb_entries){
    struct nm_so_stack * stack
        = TBX_MALLOC(sizeof(struct nm_so_stack));
    stack->nb_entries = nb_entries;
    stack->obj = TBX_MALLOC(nb_entries * sizeof(void *));
    stack->next_to_add = 0;
    return stack;
}

void
nm_so_stack_free(struct nm_so_stack *stack){
    TBX_FREE(stack->obj);
    TBX_FREE(stack);
}

void
nm_so_stack_push(struct nm_so_stack *stack, void *obj){
    assert(stack->next_to_add < stack->nb_entries);
    stack->obj[stack->next_to_add] = obj;
    stack->next_to_add++;
}

void *
nm_so_stack_pop(struct nm_so_stack *stack){
    assert(stack->next_to_add);
    void * obj = stack->obj[stack->next_to_add - 1];
    stack->next_to_add--;
    return obj;
}

int
nm_so_stack_size(struct nm_so_stack *stack){
    return stack->next_to_add;
}

void *
nm_so_stack_top(struct nm_so_stack *stack){
    DISP_VAL("nm_so_stack_top - next_to_add", stack->next_to_add);
    assert(stack->next_to_add);
    return stack->obj[stack->next_to_add - 1];
}


void *
nm_so_stack_down(struct nm_so_stack *stack){
    assert(stack->next_to_add);
    return stack->obj[0];
}
