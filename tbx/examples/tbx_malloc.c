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
struct s_object {
	int	 a;
	void	*b;
};

int
main(int    argc,
		char **argv) {
	void	*t[N];
	int	 i;

	tbx_init(argc, argv);
	tbx_malloc_init(&my_allocator, sizeof(struct s_object), 0, "appli/my_allocator");

	printf("Phase 1\n");
	for (i = 0; i < N; i++) {
		t[i] = tbx_malloc(my_allocator);
		printf("t[%d] = %p\n", i, t[i]);
	}

	for (i = 0; i < N; i++) {
		tbx_free(my_allocator, t[i]);
	}

	printf("Phase 2\n");
	for (i = 0; i < N; i++) {
		t[i] = tbx_malloc(my_allocator);
		printf("t[%d] = %p\n", i, t[i]);
	}

	for (i = 0; i < N; i++) {
		tbx_free(my_allocator, t[i]);
	}

	tbx_malloc_clean(my_allocator);

	return 0;
}

