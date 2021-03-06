
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>


#include "tbx.h"
#include "fut_pm2.h"


#ifdef MARCEL
#include "marcel.h"
#define PROF_THREAD_ID()  (marcel_thread_number(marcel_self()))
#else // MARCEL
#define PROF_THREAD_ID()  0
#endif

#define PROF_BUFFER_SIZE  (16*1024*1024)

volatile tbx_bool_t __pm2_profile_active = tbx_false;
static char PROF_FILE_USER[1024];
static tbx_bool_t profile_initialized = tbx_false;
static tbx_bool_t activate_called_before_init =
#ifdef PROFILE_AUTOSTART
	tbx_true;
#else
tbx_false;
#endif
static struct {
	int how;
	unsigned user_keymask, kernel_keymask;
	unsigned thread_id;
} activate_params
#ifdef PROFILE_AUTOSTART
= { FUT_ENABLE, FUT_KEYMASKALL, FUT_KEYMASKALL, 0 };
#endif
;

void profile_init(void)
{
	if(!profile_initialized) {

		profile_initialized = tbx_true;
		__pm2_profile_active = tbx_true;

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

		atexit(profile_stop);
	}
}

void profile_activate(int how, unsigned user_keymask, unsigned kernel_keymask)
{
	if(profile_initialized)
		fut_keychange(how, user_keymask, PROF_THREAD_ID());
	else {
		activate_params.how = how;
		activate_params.user_keymask = user_keymask;
		activate_params.kernel_keymask = kernel_keymask;
		activate_params.thread_id = PROF_THREAD_ID();
		activate_called_before_init = tbx_true;
	}

}

void profile_set_tracefile(char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	vsprintf(PROF_FILE_USER, fmt, vl);
	va_end(vl);

	char pid[11];
	snprintf(pid, 11, "_%d", getpid ());

	strcat(PROF_FILE_USER, "_pm2user_");
	strcat(PROF_FILE_USER, getenv("USER"));
	strcat(PROF_FILE_USER, pid);
}

void profile_stop(void)
{
	if (__pm2_profile_active)
		fut_endup(PROF_FILE_USER);

	__pm2_profile_active = tbx_false;
}

void profile_exit(void)
{
	profile_stop();
	fut_done();
}
