/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

#include <stdio.h>
#include "tbx.h"
#define N 10

p_tbx_memory_t my_allocator;
struct s_wrap {
	int	 v;
};

static
void *
wrap(int i) {
	struct s_wrap *w = tbx_malloc(my_allocator);
	w->v = i;
	return w;
}

static
void
free_wrap(struct s_wrap *w) {
	tbx_free(my_allocator, w);
}

static
void
next(p_tbx_slist_t l) {
	struct s_wrap *a;
	struct s_wrap *b;
	struct s_wrap *c;

	b = tbx_slist_pop(l);
	a = tbx_slist_pop(l);

	c = wrap(a->v + b->v);

	tbx_slist_push(l, a);
	tbx_slist_push(l, b);
	tbx_slist_push(l, c);
}

int
main(int    argc,
		char **argv) {
	p_tbx_slist_t	l;
	int	 i;

	tbx_init(argc, argv);
	tbx_malloc_init(&my_allocator, sizeof(struct s_wrap), 0, "appli/my_allocator");

	l = tbx_slist_nil();
	tbx_slist_push(l, wrap(0));
	tbx_slist_push(l, wrap(1));

	for (i = 0; i < N; i++) {
		next(l);
	}

	while (!tbx_slist_is_nil(l)) {
		struct s_wrap *w = tbx_slist_extract(l);
		printf("%d\n", w->v);
		tbx_free(my_allocator, w);
	}

#if 0
	/* enable the piece of code to make slist element allocator complain */
	tbx_slist_push(l, NULL);
#endif

	tbx_slist_free(l);
	tbx_malloc_clean(my_allocator);
	tbx_exit();

	return 0;
}

