
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

#include "pioman.h"

#if defined(PIOM_THREAD_ENABLED)
__tbx_inline__ void piom_sem_P(piom_sem_t *sem){
#ifdef MARCEL
	marcel_sem_P(sem);
#else
	while (sem_wait(sem) == -1);
#endif
}

__tbx_inline__ void piom_sem_V(piom_sem_t *sem){
#ifdef MARCEL
	marcel_sem_V(sem);
#else
	sem_post(sem);
#endif
}

__tbx_inline__ void piom_sem_init(piom_sem_t *sem, int initial){
#ifdef MARCEL
	marcel_sem_init(sem, initial);
#else
	sem_init(sem, 0, initial);
#endif
}

#define TIME_TO_POLL 20

__tbx_inline__ void piom_cond_wait(piom_cond_t *cond, uint8_t mask) {
	LOG_IN();
#ifdef MARCEL
	if(cond->value & mask)
		return ;

	/* First, let's poll for a while before blocking */
#if 1
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	do {
		__piom_check_polling(PIOM_POLL_WHEN_FORCED);
		if(cond->value & mask)
			return ;
		TBX_GET_TICK(t2);
	}while(TBX_TIMING_DELAY(t1, t2)<TIME_TO_POLL);
#else
	int i;
	for(i=0;i<30;i++) {
		__piom_check_polling(PIOM_POLL_WHEN_FORCED);
		if(cond->value & mask)
			return ;
	}
#endif
#ifdef MARCEL
	/* set highest priority so that the thread 
	   is scheduled (almost) immediatly when done */
	struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
	struct marcel_sched_param old_param;
	marcel_sched_getparam(MARCEL_SELF, &old_param);
	marcel_sched_setparam(MARCEL_SELF, &sched_param);
#endif

	while(! (cond->value & mask)){
		cond->cpt++;
		piom_sem_P(&cond->sem);
		cond->cpt--;
		if(cond->cpt)
			/* another thread is waiting for the same semaphore */
			piom_sem_V(&cond->sem);
	}

#ifdef MARCEL
	marcel_sched_setparam(MARCEL_SELF, &old_param);
#endif
#else
	/* no Marcel, do not block as possibly nobody will wake us... (We have neither timers nor supplementary VP) */
	while(! (cond->value & mask))
		__piom_check_polling(PIOM_POLL_AT_IDLE);		
#endif
	LOG_OUT();
}

__tbx_inline__ void piom_cond_signal(piom_cond_t *cond, uint8_t mask){
	LOG_IN();
	cond->value|=mask;
	piom_sem_V(&cond->sem);
	/* TODO: make this optional to save a few cycles */
#ifdef PIOM_ENABLE_SHM
	if(cond->alt_sem){
		piom_shs_poll_success(cond->alt_sem);
	}
#endif
	LOG_OUT();
}

__tbx_inline__ int piom_cond_test(piom_cond_t *cond, uint8_t mask){
	return cond->value & mask;
}

__tbx_inline__ void piom_cond_init(piom_cond_t *cond, uint8_t initial){
	
	cond->value=initial;
#ifdef PIOM_ENABLE_SHM
	cond->alt_sem=NULL;
#endif
	cond->cpt=0;
	piom_sem_init(&cond->sem, 0);
	piom_spin_lock_init(&cond->lock);
#ifdef MARCEL
	marcel_cond_init(&cond->cond,NULL);
#endif
}

__tbx_inline__ void piom_cond_mask(piom_cond_t *cond, uint8_t mask){
	cond->value&=mask;
}

#ifdef PIOM_ENABLE_SHM
int piom_cond_attach_sem(piom_cond_t *cond, piom_sh_sem_t *sh_sem){
	if(cond->alt_sem){
		return 1;
	}
	/* do not initialize it since it can be already initialized */
	cond->alt_sem = sh_sem;
	return 0;
}
#endif	/* PIOM_ENABLE_SHM */

#else  /* MARCEL */

/* Warning: we assume that Marcel is not running and there is no thread here
 * these functions are not reentrant
 */
__tbx_inline__ void piom_sem_P(piom_sem_t *sem){
	(*sem)--;
	while((*sem) < 0){
		__piom_check_polling(PIOM_POLL_AT_IDLE);
	}
}

__tbx_inline__ void piom_sem_V(piom_sem_t *sem){
	(*sem)++;
}

__tbx_inline__ void piom_sem_init(piom_sem_t *sem, int initial){
	(*sem)=initial;
}

__tbx_inline__ void piom_cond_wait(piom_cond_t *cond, uint8_t mask){
	while(! (*cond & mask))
		__piom_check_polling(PIOM_POLL_AT_IDLE);		
}
__tbx_inline__ void piom_cond_signal(piom_cond_t *cond, uint8_t mask){
	*cond |= mask;
}
__tbx_inline__ int piom_cond_test(piom_cond_t *cond, uint8_t mask){
	return *cond & mask;
}
__tbx_inline__ void piom_cond_init(piom_cond_t *cond, uint8_t initial){
	*cond = initial;
}

__tbx_inline__ void piom_cond_mask(piom_cond_t *cond, uint8_t mask){
	*cond &=mask;
}

#ifdef PIOM_ENABLE_SHM
int piom_cond_attach_sem(piom_cond_t *cond, piom_sh_sem_t *sh_sem){
/* TODO: fix me! alt_sem is not defined */
//	if(cond->alt_sem)
//		return 1;
//	/* do not initialize it since it can be already initialized */
//	cond->alt_sem = sh_sem;
	return 0;
}
#endif
#endif /* PIOM_THREAD_ENABLED */
