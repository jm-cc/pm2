
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

#section macros
#define MA_JIFFIES_PER_TIMER_TICK 1

#section macros
#ifdef MA__TIMER
#define JIFFIES_FROM_US(microsecs) \
  ((microsecs)*MA_JIFFIES_PER_TIMER_TICK/marcel_gettimeslice())
#else
#define JIFFIES_FROM_US(microsecs) \
  ((microsecs)*MA_JIFFIES_PER_TIMER_TICK/10000)
#endif

#section functions
void marcel_settimeslice(unsigned long microsecs);
unsigned long marcel_gettimeslice();
unsigned long marcel_clock(void);

#section marcel_functions
void marcel_sig_exit(void);
void marcel_sig_pause(void);
void marcel_sig_enable_interrupts(void);
void marcel_sig_disable_interrupts(void);

#section marcel_functions
static __inline__ void disable_preemption(void);
static __inline__ void enable_preemption(void);
static __inline__ unsigned int preemption_enabled(void);
#section marcel_variables
extern ma_atomic_t __preemption_disabled;
#section marcel_inline
static __inline__ void disable_preemption(void)
{
	ma_atomic_inc(&__preemption_disabled);
}

static __inline__ void enable_preemption(void)
{
	ma_atomic_dec(&__preemption_disabled);
}

static __inline__ unsigned int preemption_enabled(void)
{
	return ma_atomic_read(&__preemption_disabled) == 0;
}
