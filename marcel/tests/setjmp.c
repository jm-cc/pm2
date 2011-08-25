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

/* Test the behavior of Marcel wrt. setjmp/longjmp(3).  */
#include "marcel.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>


#ifdef MARCEL_LIBPTHREAD


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	jmp_buf buf;
	sigjmp_buf sigbuf;
	int jmp;
	int sigjmp;

	jmp = 1;
	if (!setjmp(buf)) {
#ifdef VERBOSE_TESTS
		printf("long jump 1\n");
#endif
		jmp = 0;
		longjmp(buf, 1);
	}
	jmp = ~jmp;
	if (!jmp)
		fprintf(stderr, "longjump  failed\n");

	sigjmp = 1;
	if (!sigsetjmp(sigbuf, 1)) {
#ifdef VERBOSE_TESTS
		printf("long jump 2\n");
#endif
		sigjmp = 0;
		siglongjmp(sigbuf, 1);
	}
	sigjmp = ~sigjmp;
	if (!sigjmp)
		fprintf(stderr, "siglongjump failed\n");

	return (jmp && sigjmp) ? MARCEL_TEST_SUCCEEDED : 1;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
