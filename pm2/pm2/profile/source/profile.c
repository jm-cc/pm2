
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#ifndef X86_ARCH
#error "PROFILE FACILITIES ARE CURRENTLY ONLY AVAILABLE ON X86 ARCHITECTURES"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "profile.h"

#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_entries.h"
#endif

#ifdef MARCEL

#include "marcel.h"

#define PROF_THREAD_ID()  (marcel_self()->number)

#else // MARCEL

#define PROF_THREAD_ID()  0

#endif

#define PROF_BUFFER_SIZE  (2*1024*1024)

static char PROF_FILE[1024];

static boolean profile_initialized = FALSE;
static boolean activate_called_before_init = FALSE;
static struct {
  int how;
  unsigned keymask;
  unsigned thread_id;
} activate_params;

void profile_init(void)
{
  if(!profile_initialized) {

    strcpy(PROF_FILE, "/tmp/prof_file_single");

    if(fut_setup(PROF_BUFFER_SIZE, FUT_KEYMASKALL, PROF_THREAD_ID()) < 0) {
      perror("fut_setup");
      exit(EXIT_FAILURE);
    }

    profile_initialized = TRUE;

    if(activate_called_before_init)
      fut_keychange(activate_params.how,
		    activate_params.keymask,
		    activate_params.thread_id);
    else
      fut_keychange(FUT_DISABLE, FUT_KEYMASKALL, PROF_THREAD_ID());
  }
}

void profile_activate(int how, unsigned keymask)
{
  if(profile_initialized) {

    fut_keychange(how, keymask, PROF_THREAD_ID());

  } else {

    activate_params.how = how;
    activate_params.keymask = keymask;
    activate_params.thread_id = PROF_THREAD_ID();

    activate_called_before_init = TRUE;

  }
}

void profile_set_tracefile(char *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  vsprintf(PROF_FILE, fmt, vl);
  va_end(vl);
}

void profile_stop(void)
{
  fut_endup(PROF_FILE);
}

