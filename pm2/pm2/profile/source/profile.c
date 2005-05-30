
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

#if ! defined(X86_ARCH) && ! defined(IA64_ARCH)
#error "PROFILE FACILITIES ARE CURRENTLY ONLY AVAILABLE ON X86 AND IA64 ARCHITECTURES"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "pm2_profile.h"
#include "fxt/fxt.h"
#include "pm2_fxt-tools.h"

#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_entries.h"
#endif

#ifdef USE_FKT
#include "fkt.h"
#include "fkt_pm2.h"
#endif

#ifdef MARCEL

#include "marcel.h"

#define PROF_THREAD_ID()  (marcel_self()->number)

#else // MARCEL

#define PROF_THREAD_ID()  0

#endif

#define PROF_BUFFER_SIZE  (2*1024*1024)

volatile unsigned __pm2_profile_active = FALSE;
int fkt_ok;


static char PROF_FILE_USER[1024];

#ifdef USE_FKT
static char PROF_FILE_KERNEL[1024];
#endif

static boolean profile_initialized = FALSE;
static boolean activate_called_before_init = FALSE;
static struct {
  int how;
  unsigned user_keymask, kernel_keymask;
  unsigned thread_id;
} activate_params;

void profile_init(void)
{
  if(!profile_initialized) {

    profile_initialized = TRUE;
    __pm2_profile_active = TRUE;

    // Initialisation de FUT

    profile_set_tracefile("/tmp/prof_file");
    
    if(fut_setup(PROF_BUFFER_SIZE, FUT_KEYMASKALL, PROF_THREAD_ID()) < 0) {
      perror("fut_setup");
      exit(EXIT_FAILURE);
    }

    fut_get_mysymbols(fut);
		
    if(activate_called_before_init)
      fut_keychange(activate_params.how,
		    activate_params.user_keymask,
		    activate_params.thread_id);
    else
      fut_keychange(FUT_DISABLE, FUT_KEYMASKALL, PROF_THREAD_ID());

#ifdef USE_FKT
    // Initialisation de FKT

    if ((fkt_ok=!(fkt_record(PROF_FILE_KERNEL,0,0,0)))) {

      if(activate_called_before_init) {
        fkt_keychange(activate_params.how,
		      activate_params.kernel_keymask);
      } else
        fkt_keychange(FKT_DISABLE, FKT_KEYMASKALL);
    } else
      fprintf(stderr,"fkt failed\n");
#endif

  }
}

void profile_activate(int how, unsigned user_keymask, unsigned kernel_keymask)
{
  if(profile_initialized) {

    fut_keychange(how, user_keymask, PROF_THREAD_ID());

#ifdef USE_FKT
    if (fkt_ok)
      fkt_keychange(how, kernel_keymask);
#endif

  } else {

    activate_params.how = how;
    activate_params.user_keymask = user_keymask;
    activate_params.kernel_keymask = kernel_keymask;
    activate_params.thread_id = PROF_THREAD_ID();

    activate_called_before_init = TRUE;

  }

}

void profile_set_tracefile(char *fmt, ...)
{
  va_list vl;
  //char *prof_id;

  //prof_id=getenv("PM2_PROFILE_ID");
  //if (prof_id) {
    //fprintf(stderr, "Using ID : %s\n", prof_id);
  //}

  va_start(vl, fmt);
  vsprintf(PROF_FILE_USER, fmt, vl);
#ifdef USE_FKT
  strcpy(PROF_FILE_KERNEL, PROF_FILE_USER);
#endif
  va_end(vl);
  strcat(PROF_FILE_USER, "_user_");
  strcat(PROF_FILE_USER, getenv("USER"));
  //if (prof_id) {
  //  strcat(PROF_FILE_USER, prof_id);
  //}
#ifdef USE_FKT
  strcat(PROF_FILE_KERNEL, "_kernel_");
  strcat(PROF_FILE_KERNEL, getenv("USER"));
  //if (prof_id) {
  //  strcat(PROF_FILE_KERNEL, prof_id);
  //}
#endif
}

void profile_stop(void)
{
  __pm2_profile_active = FALSE;

  fut_endup(PROF_FILE_USER);
}

void profile_exit(void)
{
#ifdef USE_FKT
  if (fkt_ok)
    fkt_stop();
#endif

  fut_done();
}
