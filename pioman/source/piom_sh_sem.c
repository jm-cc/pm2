
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

/* list of pending sh_sems that need to be polled */
#ifdef PIOM_ENABLE_SHM

static TBX_FAST_LIST_HEAD(piom_list_shs);

int piom_shs_polling_is_required() {
	return !tbx_fast_list_empty(&piom_list_shs);
}

#ifdef MARCEL
static piom_rwlock_t piom_shs_lock = PIOM_RW_LOCK_UNLOCKED;

/* polling function called by pioman */
int piom_shs_poll(){
	struct piom_sh_sem *cur_sem, *prev_sem=NULL;

 start:
	piom_read_lock(&piom_shs_lock);

	tbx_fast_list_for_each_entry_safe(cur_sem, 
				 prev_sem, 
				 &piom_list_shs, 
				 pending_shs) {		

		if(piom_shs_test(cur_sem)){
			piom_read_unlock(&piom_shs_lock);			
			goto retry;
		}
	}
	piom_read_unlock(&piom_shs_lock);			
	
	return 0;

 retry:	
	piom_write_lock(&piom_shs_lock);
	tbx_fast_list_del_init(&(cur_sem)->pending_shs);
	piom_sem_V(&(cur_sem)->local_sem);     			
	piom_write_unlock(&piom_shs_lock);
	goto start;
}


int piom_shs_init(piom_sh_sem_t *sem){
	//ma_atomic_init(&sem->value, 0);
	sem->value ++;
	piom_sem_init(&sem->local_sem, 0);

	piom_write_lock(&piom_shs_lock);
	TBX_INIT_FAST_LIST_HEAD(&sem->pending_shs);
	piom_write_unlock(&piom_shs_lock);

	return 0;
}

int piom_shs_P(piom_sh_sem_t *sem){
	//if(ma_atomic_dec_return(&sem->value) >= 0)
	if(sem->value -- >= 0)
		goto out;	

	//ma_atomic_inc(&sem->value);
	sem->value ++;

	/* first: poll for one usec */
	/* TODO: make this optional */
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	do{
		piom_shs_poll();
		//if(ma_atomic_read(&sem->value)>0)
		if(sem->value>0)
			goto out;
		TBX_GET_TICK(t2);
	}while(TBX_TIMING_DELAY(t1, t2)<1);

	//while(ma_atomic_read(&sem->value)<=0){
	while(sem->value <= 0){
		piom_write_lock(&piom_shs_lock);
		tbx_fast_list_add_tail(&sem->pending_shs, &piom_list_shs);
		piom_write_unlock(&piom_shs_lock);

		piom_sem_P(&sem->local_sem);
		
		piom_write_lock(&piom_shs_lock);
		tbx_fast_list_del_init(&(sem)->pending_shs);
		piom_write_unlock(&piom_shs_lock);
		
		/* check wether we didn't wake up for nothing */
		//if(ma_atomic_dec_return(&sem->value) >= 0)
		if(sem->value-- >= 0)
			goto out;		
		sem->value ++;
	}	
out:
	return 0;
}

int piom_shs_V(piom_sh_sem_t *sem){
	sem->value++;
	/* todo: add a kind of signal for blocking calls */
	return 0;
}

int piom_shs_test(piom_sh_sem_t *sem){
	return (sem->value >=1);
}

#else  /* MARCEL */

int piom_shs_poll(){
	/* nothing to do here since we poll
	   in piom_shs_P  */
}

int piom_shs_init(piom_sh_sem_t *sem){
	sem->value = 0;
	return 0;
}

int piom_shs_P(piom_sh_sem_t *sem){
	if(--(sem->value)>=0)
		return 0;
	(sem->value)++;
	while(sem->value<=0){
		__piom_check_polling(PIOM_POLL_AT_IDLE);
		if(--(sem->value)>=0)
			return 0;
		(sem->value)++;
	}
	return 0;
}

int piom_shs_V(piom_sh_sem_t *sem){
	sem->value++;
	return 0;
}

int piom_shs_test(piom_sh_sem_t *sem){
	return (sem->value >=1);
}

#endif	/* MARCEL */



#endif /* PIOM_ENABLE_SHM */
