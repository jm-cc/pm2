/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

/* Test whether `marcel_test_activity ()' behaves as expected.  */

#include <marcel.h>

#undef NDEBUG
#include <assert.h>


int main(int argc, char *argv[])
{
#ifdef STACKALIGN
	if (marcel_test_activity()) {
		fprintf(stderr, "marcel is not yet initialized\n");
		return 1;
	}
#endif

	marcel_init(&argc, argv);
	if (!marcel_test_activity()) {
		fprintf(stderr, "marcel must be initialized\n");
		return 1;
	}

	marcel_end();
	if (marcel_test_activity()) {
		fprintf(stderr, "marcel must not be initialized\n");
		return 1;
	}

	return 0;
}
