
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

#ifndef MARCEL_SIG_IS_DEF
#define MARCEL_SIG_IS_DEF

#include <signal.h>

#include "marcel.h"

// Signal utilisé pour la préemption automatique
#ifdef USE_VIRTUAL_TIMER

#define MARCEL_TIMER_SIGNAL   SIGVTALRM
#define MARCEL_ITIMER_TYPE    ITIMER_VIRTUAL

#else

#define MARCEL_TIMER_SIGNAL   SIGALRM
#define MARCEL_ITIMER_TYPE    ITIMER_REAL

#endif

void marcel_sig_init(void);

void marcel_sig_pause(void);

void marcel_sig_enable_interrupts(void);
void marcel_sig_disable_interrupts(void);

void marcel_sig_reset_timer(void);
void marcel_sig_start_timer(void);
void marcel_sig_stop_timer(void);

unsigned long marcel_clock(void);
void marcel_update_time(marcel_t cur);
void marcel_settimeslice(unsigned long microsecs);

#endif

