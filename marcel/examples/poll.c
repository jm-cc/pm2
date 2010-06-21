
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
#include <stdio.h>
#include <unistd.h>

static int tube[2];

any_t f(any_t arg)
{
  char chaine[1024];
  int i;

  marcel_detach(marcel_self());

  marcel_delay(100);

  marcel_printf("I'm waiting for a keyboard input\n");
  do {
    i = marcel_read(STDIN_FILENO, chaine, 1024);
  } while (i<=0);
  chaine[i] = 0;
  marcel_printf("Input from [keyboard] detected on LWP %d: %s",
		marcel_current_vp(), chaine);

  return NULL;
}

any_t g(any_t arg)
{
  unsigned i;

  marcel_detach(marcel_self());

  marcel_delay(100);

  marcel_printf("I'm waiting for a pipe input\n");
  marcel_read(tube[0], &i, sizeof(i));
  marcel_printf("Input from [pipe] detected on LWP %d: %x\n",
		marcel_current_vp(), i);

  return NULL;
}

any_t g2(any_t arg)
{
  unsigned i;

  marcel_detach(marcel_self());

  marcel_delay(50);

  marcel_printf("I'm waiting for a pipe input 2\n");
  marcel_read(tube[0], &i, sizeof(i));
  marcel_printf("Input from [pipe 2] detected on LWP %d: %x\n",
		marcel_current_vp(), i);

  return NULL;
}

volatile int out=0;

any_t h(any_t arg)
{
	marcel_detach(marcel_self());
	while (!out) ;
	return NULL;
}

int marcel_main(int argc, char *argv[])
{
  volatile unsigned long i;
  struct {
	  unsigned a;
	  unsigned b;
  } magic = { 0xdeadbeef, 0xfecabead };

  marcel_trace_on();

  marcel_init(argc, argv);

  pipe(tube);

  //marcel_create(NULL, NULL, h, NULL);
  marcel_create(NULL, NULL, f, NULL);
  marcel_delay(500);
  marcel_create(NULL, NULL, g, NULL);
  marcel_delay(100);
  marcel_create(NULL, NULL, g2, NULL);

  marcel_printf("busy waiting...\n");

  i = 200000000L; while(--i);

  marcel_printf("marcel_delay...\n");

  marcel_delay(5000);

  marcel_printf("writing in pipe...\n");
  marcel_write(tube[1], &magic, sizeof(magic));

  out=1;
  marcel_end();
  return 0;
}

