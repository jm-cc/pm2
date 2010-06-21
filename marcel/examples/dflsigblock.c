
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

/* dflsigblock.c */

#include "marcel.h"

#ifdef MARCEL_SIGNALS_ENABLED

static void handler(int sig) {
  marcel_printf("got signal %d\n", sig);
}

static const marcel_sigset_t set = marcel_sigoneset(SIGUSR1) | marcel_sigoneset(SIGUSR2);
static int done;

static any_t f(any_t foo) {
  marcel_sigmask(SIG_BLOCK, &set, NULL);
  marcel_kill(marcel_self(), SIGUSR1);
  marcel_kill(marcel_self(), SIGUSR2);
  marcel_signal(SIGUSR2, SIG_IGN);
  marcel_yield();
  marcel_yield();
  marcel_sigmask(SIG_UNBLOCK, &set, NULL);
  /* shouldn't have crashed, as we have reset to SIG_IGN */
  done = 1;
  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_init(argc, argv);

  marcel_signal(SIGUSR1, handler);
  marcel_signal(SIGUSR2, SIG_DFL);

  marcel_create(NULL, NULL, f, NULL);

  while (!done)
    marcel_yield();

  marcel_end();
  return 0;
}
#else /* MARCEL_SIGNALS_ENABLED */
#  warning Marcel signals must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel signals' feature disabled in the flavor\n");

  return 0;
}
#endif /* MARCEL_SIGNALS_ENABLED */
