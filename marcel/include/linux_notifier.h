/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __LINUX_NOTIFIER_H__
#define __LINUX_NOTIFIER_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"


/** Public data types **/
struct ma_notifier_block;
struct ma_notifier_chain;


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define MA_DEFINE_NOTIFIER_BLOCK(var, func, help) MA_DEFINE_NOTIFIER_BLOCK_PRIO(var, func, 0, help)
#define MA_DEFINE_NOTIFIER_BLOCK_PRIO(var, func, prio, help) \
  MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(var, func, prio, help, 0, NULL)
#define MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(var, func, prio, help, nb, helps) \
  struct ma_notifier_block var = { \
        .notifier_call = (func), \
        .next           = NULL, \
        .priority       = prio, \
        .name           = help, \
        .nb_actions     = nb, \
        .actions_name   = helps, \
  }

#define MA_DEFINE_NOTIFIER_CHAIN(var, help) \
  struct ma_notifier_chain var = { \
        .chain = NULL, \
        .name  = help, \
  }

#define MA_NOTIFY_DONE		0x0000		/* Don't care */
#define MA_NOTIFY_OK		0x0001		/* Suits me */
#define MA_NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define MA_NOTIFY_BAD		(MA_NOTIFY_STOP_MASK|0x0002)	/* Bad/Veto action	*/

/*
 *	Declared notifiers so far. I can imagine quite a few more chains
 *	over time (eg laptop power reset chains, reboot chain (to clean 
 *	device units up), device [un]mount chain, module load/unload chain,
 *	low memory chain, screenblank chain (for plug in modular screenblankers) 
 *	VC switch chains (for loadable kernel svgalib VC switch helpers) etc...
 */ 
#define MA_LWP_ONLINE		0x0002 /* LWP (ma_lwp_t)v is up */
#define MA_LWP_UP_PREPARE	0x0003 /* LWP (ma_lwp_t)v coming up */
#define MA_LWP_UP_CANCELED	0x0004 /* LWP (ma_lwp_t)v NOT coming up */
#define MA_LWP_OFFLINE		0x0005 /* LWP (ma_lwp_t)v offline (still scheduling) */
#define MA_LWP_DEAD		0x0006 /* LWP (ma_lwp_t)v dead */


/** Internal data structures **/
struct ma_notifier_block
{
	int (*notifier_call)(struct ma_notifier_block *self, unsigned long, void *);
	struct ma_notifier_block *next;
	int priority;
	const char* name;
	int nb_actions;
	const char **actions_name;
};

struct ma_notifier_chain
{
	struct ma_notifier_block *chain;
	const char* name;
};


/** Internal functions **/
extern int ma_notifier_chain_register(struct ma_notifier_chain *c, struct ma_notifier_block *n);
extern int ma_notifier_chain_unregister(struct ma_notifier_chain *c, struct ma_notifier_block *n);
extern int ma_notifier_call_chain(struct ma_notifier_chain *c, unsigned long val, void *v);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_NOTIFIER_H__ **/
