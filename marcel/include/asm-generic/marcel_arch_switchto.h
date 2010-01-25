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


#ifndef __MARCEL_ARCH_SWITCHTO_H__
#define __MARCEL_ARCH_SWITCHTO_H__


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* On se sert de ces macros si on veut détecter et/ou stopper la préemption
 * lors des signaux 
 * Voir l'architecture ia64... */

#define MA_ARCH_SWITCHTO_LWP_FIX(current) ((void)0)

/* Les deux macros suivantes doivent être utilisées par pair 
 * (elles peuvent définir un block {})
 */
#define MA_ARCH_INTERRUPT_ENTER_LWP_FIX(current, p_ucontext_t) ((void)0)
#define MA_ARCH_INTERRUPT_EXIT_LWP_FIX(current, p_ucontext_t) ((void)0)


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_ARCH_SWITCHTO_H__ **/
