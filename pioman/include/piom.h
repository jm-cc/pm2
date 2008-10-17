/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_H
#define PIOM_H

#include "pm2_list.h"


/* List of servers currently doing some polling
 * (state == 2 and tasks waiting for polling) 
 */
extern struct list_head piom_list_poll;
#ifdef MARCEL
extern ma_spinlock_t piom_poll_lock;
extern int job_scheduled;
#else
extern void* piom_poll_lock;
#endif

/* The server defines callbacks and polling parameters. It may group requests */
typedef struct piom_server *piom_server_t;

/* A request defines an event that can be detected. Several requests may
 * be grouped (to permit to poll only once for a group of requests)
 */
typedef struct piom_req *piom_req_t;

/* A thread can wait for an event corresponding to a request. The thread
 * is descheduled until the request is completed (ie: the event happened) */
typedef struct piom_wait *piom_wait_t;

#ifndef MARCEL
/* Without Marcel compliance */
#define piom_time_t int*
#else
#define piom_time_t marcel_time_t
#endif	/* MARCEL */

/* Available kinds of callbacks */
typedef enum {
	/* the callback polls a specific request */
	PIOM_FUNCTYPE_POLL_POLLONE,
	/* the callback groups requests */
	PIOM_FUNCTYPE_POLL_GROUP,
	/* the callback polls every submitted request */
	PIOM_FUNCTYPE_POLL_POLLANY,
	/* the callback exports a syscall to complete a specific request */
	PIOM_FUNCTYPE_BLOCK_WAITONE,
	/* idem with a timeout */
	PIOM_FUNCTYPE_BLOCK_WAITONE_TIMEOUT,
	/* the callback groups requests to be exported */
	PIOM_FUNCTYPE_BLOCK_GROUP,
	/* the callback completes every submitted requests using an exported syscall */
	PIOM_FUNCTYPE_BLOCK_WAITANY,
	/* idem with a timeout */
	PIOM_FUNCTYPE_BLOCK_WAITANY_TIMEOUT,
	/* unblocks a request blocked with PIOM_FUNCTYPE_BLOCK_WAITONE */
	PIOM_FUNCTYPE_UNBLOCK_WAITONE,
	/* unblocks a request blocked with PIOM_FUNCTYPE_BLOCK_WAITANY */
	PIOM_FUNCTYPE_UNBLOCK_WAITANY,
	/* PRIVATE */
	PIOM_FUNCTYPE_SIZE
} piom_op_t;

/* Callback's speed */
typedef enum {
	PIOM_CALLBACK_SLOWEST = 1,	// >10 탎
	PIOM_CALLBACK_SLOW = 2,	// several 탎
	PIOM_CALLBACK_NORMAL_SPEED = 3,	// 1 탎
	PIOM_CALLBACK_FAST = 4,	// 1/2 탎
	PIOM_CALLBACK_IMMEDIATE = 5,	// immediate
} piom_callback_speed_t;

/* Method to use to complete a request */
typedef enum {
	/* Choose automaticaly the best method */
	PIOM_FUNC_AUTO = 1,
	/* Use the polling fonction (ie, the request will probably be completed soon) */
	PIOM_FUNC_POLLING = 2,
	/* Use the syscall method (ie, the request will probably be completed later) */
	PIOM_FUNC_SYSCALL = 3,
} piom_func_to_use_t;

/********************************************************************
 * Callbacks prototype
 * server : the server
 * op : the operation (callback) requested
 * req : only for *(POLL|WAIT)ONE* : the specific request to test
 * nb_req : only for POLL_* : the amount of grouped queries
 * option : flags depending on the operation
 *  - only for POLL_POLLONE : 
 *     + PIOM_IS_GROUPED : wether the request is already grouped
 *     + PIOM_ITER : if POLL_POLLONE is called for every request
 *                 (ie POLL_POLLANY isn't available)
 *  - for BLOCK_ONE/BLOCK_ANY is the server is interruptible:
 *     + the file descriptor to wait for: if data is received through this fd
 *       the callback should return to have a new request to block for
 *
 * The ret value is ignored
 */
typedef int (piom_callback_t) (piom_server_t server,
				piom_op_t op,
				piom_req_t req, int nb_ev, int option);

typedef struct {
	piom_callback_t *func;
	piom_callback_speed_t speed;
} piom_pcallback_t;

struct piom_wait {
	/* List of groupes events we are waiting for */
	struct list_head chain_wait;

#ifdef MARCEL
	/* TODO : Useless without marcel ? */
	marcel_sem_t sem;
	marcel_task_t *task;
#endif
	/* Ret value
	   0: event
	   -ECANCELED: piom_req_cancel called
	 */
	int ret;
};

/*********************************************************************
 * Find the global structure's address
 * It needs :
 * - address of intern field
 * - type of the global structure
 * - name of the intern field of the global structure
 */
#define struct_up(ptr, type, member) \
	tbx_container_of(ptr, type, member)

/* Macro used in callbacks to wake up a waiting thread, giving it the ret value
 *   piom_wait_t wait : waiter to wake up
 */
#define PIOM_WAIT_SUCCESS(wait, code) \
  do { \
        list_del(&(wait)->chain_wait); \
        (wait)->ret_code=(code); \
  } while(0)

#define PIOM_EXCEPTION_RAISE(e) abort();

/********************************************************************
 *  INLINE FUNCTIONS
 */

static __tbx_inline__ int piom_polling_is_required(unsigned polling_point)
    TBX_UNUSED;
static __tbx_inline__ void piom_check_polling(unsigned polling_point)
    TBX_UNUSED;
void __piom_check_polling(unsigned polling_point);

/* Check wether there is some polling to do
 * Called from Marcel 
 */
static __tbx_inline__ int piom_polling_is_required(unsigned polling_point)
{
	return !list_empty(&piom_list_poll);
}

/* Try to poll
 * Called from Marcel
 */
static __tbx_inline__ void piom_check_polling(unsigned polling_point)
{
	if (piom_polling_is_required(polling_point))
		__piom_check_polling(polling_point);
}

/********************************************************************
 *  FUNCTIONS PROTOTYPES 
 */
int piom_test_activity(void);

/* TODO timeout support */
int piom_wait(piom_server_t server, piom_req_t req,
	      piom_wait_t wait, piom_time_t timeout);


/* piom_wait is a usefull function is the request is only sumitted once.
 * the following operations are performed: request initialization, submition 
 * and wait for an event (with the ONE_SHOT flag set)
 */



/* Waits until an already registered request is completed */
/* TODO : timeout support */
int piom_req_wait(piom_req_t req, piom_wait_t wait,
		   piom_time_t timeout);

/* Waits until any request is completed*/
int piom_server_wait(piom_server_t server, piom_time_t timeout);

/* Wake up threads that are waiting for a request */
int __piom_wake_req_waiters(piom_server_t server,
			    piom_req_t req, int code);

/* Wake up threads that are waiting for a server's event */
int __piom_wake_id_waiters(piom_server_t server, int code);

/* Wait for a request's completion (internal use only) */
int __piom_wait_req(piom_server_t server, piom_req_t req,
		    piom_wait_t wait, piom_time_t timeout);

#endif /* PIOM_H */


