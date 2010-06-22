
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/* timeout.c */

#include "marcel.h"

static
marcel_sem_t sem;

static
void *
f(void *arg) {
	for (;;) {
		if (marcel_sem_timed_P(&sem, 500))
			break;

        	marcel_printf("What a boring job, isn't it ?\n");
	}
	marcel_printf("What a relief !\n");

	return 0;
}

int
marcel_main(int argc, char *argv[]) {
	void *status;
	marcel_t pid;

	marcel_init(&argc, argv);

	marcel_sem_init(&sem, 0);

	marcel_create(&pid, NULL, f,  NULL);

	marcel_delay(2250);
	marcel_sem_V(&sem);

	marcel_join(pid, &status);

	marcel_end();
	return 0;
}
