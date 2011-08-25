
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


#include "marcel.h"
#include "marcel_alloc.h"


#if defined(ENABLE_STACK_JUMPING) && defined(WHITE_BOX)


static void *stack;

static
any_t thread_func(any_t arg TBX_UNUSED)
{
	marcel_detach(marcel_self());

#ifdef VERBOSE_TESTS
	marcel_printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *) get_sp());
#endif

	/* Allocation d'une pile annexe, *obligatoirement* de taille THREAD_SLOT_SIZE.  */
	stack = marcel_slot_alloc(NULL);
#ifdef VERBOSE_TESTS
	marcel_printf("New stack goes from %p to %p\n", stack,
		      stack + THREAD_SLOT_SIZE - 1);
#endif

	/* Préparation de la nouvelle pile pour accueillir le thread */
	marcel_prepare_stack_jump(stack);

	/* basculement sur la pile annexe */
	set_sp(stack + THREAD_SLOT_SIZE - 1024);

#ifdef VERBOSE_TESTS
	marcel_printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *) get_sp());
#endif

	/* on ne peut pas retourner avec "return", donc : */
	marcel_exit(NULL);

	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	marcel_init(&argc, argv);
	marcel_create(NULL, NULL, thread_func, NULL);
	marcel_end();
	return MARCEL_TEST_SUCCEEDED;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif				/* ENABLE_STACK_JUMPING */
