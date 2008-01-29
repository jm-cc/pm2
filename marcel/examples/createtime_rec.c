/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU * General Public License for more details.
 */

/* Programme d'essai pour evaluer le cout de creation de threads */

#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include <stdio.h>
#include <sys/time.h>

static tbx_tick_t t1, t2;
marcel_attr_t attr;
ma_atomic_t max_threads = MA_ATOMIC_INIT(0);
marcel_sem_t sem;
unsigned nb_feuilles;


any_t f(any_t arg)
{
        any_t n;
        n = arg;
        
        if(n != 0)
                {
                        marcel_create(NULL, &attr, f, (void *) (intptr_t) (n-1));
                        marcel_create(NULL, &attr, f, (void *) (intptr_t) (n-1));
                }
        else
                {
                        if(ma_atomic_inc_return(&max_threads) >= nb_feuilles)
                                marcel_sem_V(&sem);
                }
        
        return NULL;
}

any_t worker(any_t arg) 
{
        
        marcel_sem_init(&sem,0);
        
        marcel_attr_init(&attr);
        marcel_attr_setdetachstate(&attr, tbx_true);
        marcel_attr_setseed(&attr, 1);
        marcel_attr_setprio(&attr, MA_BATCH_PRIO);
        
        nb_feuilles = 1 << (long)arg;
        
        TBX_GET_TICK(t1);
        marcel_create(NULL, &attr, f, (any_t) arg);
        
        marcel_sem_P(&sem);
        TBX_GET_TICK(t2);
        
        marcel_printf("seed create =  %fus (%fus/seed)\n", TBX_TIMING_DELAY(t1, t2), TBX_TIMING_DELAY(t1,t2)/nb_feuilles);
               
        return NULL;
}                                                                                               


int marcel_main(int argc, char *argv[])
{
        marcel_t t;
        unsigned profondeur;
        
        marcel_init(&argc, argv);
        marcel_attr_init(&attr);
        profondeur = atoi(argv[1]);
        
        if(argc <= 1) 
                {
                        printf("Usage: %s <profondeur>\n", argv[0]);
                }
        
        /* on cree un thread main qui va creer un arbre de seed */
        marcel_create(&t, &attr, worker, (void *) (intptr_t) profondeur);
        marcel_join(t, NULL);
        
        marcel_end();
        return 0;
}
