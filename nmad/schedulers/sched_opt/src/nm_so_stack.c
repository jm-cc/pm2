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
#include <stdint.h>
#include <sys/uio.h>

#include "nm_protected.h"

struct nm_so_stack{
    int nb_entries;
    void **obj;
    int next_to_add;
};

int
nm_so_stack_create(struct nm_so_stack **pp_stack, int nb_entries){
    struct nm_so_stack * p_stack = NULL;
    int err;

    assert(nb_entries);

    p_stack = TBX_MALLOC(sizeof(struct nm_so_stack));
    if(!p_stack){
        err = -NM_ENOMEM;
        goto out;
    }

    p_stack->nb_entries = nb_entries;
    p_stack->next_to_add = 0;
    p_stack->obj = TBX_MALLOC(nb_entries * sizeof(*p_stack->obj));
    if(!p_stack->obj){
        TBX_FREE(p_stack);
        err = -NM_ENOMEM;
        goto out;
    }

    *pp_stack = p_stack;
    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_stack_free(struct nm_so_stack *p_stack){
    int err;

    assert(p_stack);

    TBX_FREE(p_stack->obj);
    TBX_FREE(p_stack);

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_stack_push(struct nm_so_stack *p_stack, void *obj){
    int err;

    assert(p_stack);
    assert(p_stack->next_to_add < p_stack->nb_entries);

    p_stack->obj[p_stack->next_to_add] = obj;
    p_stack->next_to_add++;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_stack_pop(struct nm_so_stack *p_stack, void ** pp_obj){
    void * p_obj = NULL;
    int err;

    assert(p_stack->next_to_add);

    p_obj = p_stack->obj[p_stack->next_to_add - 1];
    p_stack->next_to_add--;

    *pp_obj = p_obj;
    err = NM_ESUCCESS;

    return err;
}

int
nm_so_stack_size(struct nm_so_stack *p_stack){
    return p_stack->next_to_add;
}

int
nm_so_stack_top(struct nm_so_stack *p_stack, void **pp_obj){
    int err;

    assert(p_stack->next_to_add);

    *pp_obj = p_stack->obj[p_stack->next_to_add - 1];

    err = NM_ESUCCESS;
    return err;
}


int
nm_so_stack_down(struct nm_so_stack *p_stack, void ** pp_obj){
    int err;

    assert(p_stack->next_to_add);

    *pp_obj = p_stack->obj[0];

    err = NM_ESUCCESS;
    return err;
}
