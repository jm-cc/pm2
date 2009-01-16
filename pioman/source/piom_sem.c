
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

#ifdef MARCEL
__tbx_inline__ void piom_sem_P(piom_sem_t *sem){
	marcel_sem_P(sem);
}

__tbx_inline__ void piom_sem_V(piom_sem_t *sem){
	marcel_sem_V(sem);
}

__tbx_inline__ void piom_sem_init(piom_sem_t *sem, int initial){
	marcel_sem_init(sem, initial);
}

__tbx_inline__ void piom_cond_wait(piom_cond_t *cond, uint8_t mask) {
	LOG_IN();
	if(cond->value & mask)
		return ;
	__piom_check_polling(PIOM_POLL_WHEN_FORCED);

	/* set highest priority so that the thread 
	   is scheduled (almost) immediatly when done */
	struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
	struct marcel_sched_param old_param;
	marcel_sched_getparam(MARCEL_SELF, &old_param);
	marcel_sched_setparam(MARCEL_SELF, &sched_param);
#if 0
	while(! (cond->value & mask)){
		marcel_sem_P(&cond->sem);
	}
#else
	while(! (cond->value & mask)){
		cond->cpt++;
		marcel_sem_P(&cond->sem);
		cond->cpt--;
		if(cond->cpt)
			/* another thread is waiting for the same semaphore */
			marcel_sem_V(&cond->sem);
	}
#endif
	marcel_sched_setparam(MARCEL_SELF, &old_param);
	LOG_OUT();
}

__tbx_inline__ void piom_cond_signal(piom_cond_t *cond, uint8_t mask){
	LOG_IN();
	cond->value|=mask;
	marcel_sem_V(&cond->sem);
	/* TODO: make this optional to save a few cycles */
	if(cond->alt_sem){
		piom_shs_poll_success(cond->alt_sem);
	}
	LOG_OUT();
}

__tbx_inline__ int piom_cond_test(piom_cond_t *cond, uint8_t mask){
	return cond->value & mask;
}

__tbx_inline__ void piom_cond_init(piom_cond_t *cond, uint8_t initial){
	
	cond->value=initial;
	cond->alt_sem=NULL;
	cond->cpt=0;
	marcel_sem_init(&cond->sem, 0);
	ma_spin_lock_init(&cond->lock);
	marcel_cond_init(&cond->cond,NULL);
}

__tbx_inline__ void piom_cond_mask(piom_cond_t *cond, uint8_t mask){
	cond->value&=mask;
}

int piom_cond_attach_sem(piom_cond_t *cond, piom_sh_sem_t *sh_sem){
	if(cond->alt_sem){
		return 1;
	}
	/* do not initialize it since it can be already initialized */
	cond->alt_sem = sh_sem;
	return 0;
}

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

int piom_cond_attach_sem(piom_cond_t *cond, piom_sh_sem_t *sh_sem){
/* TODO: fix me! alt_sem is not defined */
//	if(cond->alt_sem)
//		return 1;
//	/* do not initialize it since it can be already initialized */
//	cond->alt_sem = sh_sem;
	return 0;
}

#endif /* MARCEL */
