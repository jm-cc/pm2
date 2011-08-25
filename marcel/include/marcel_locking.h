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


#ifndef __MARCEL_LOCKING_H__
#define __MARCEL_LOCKING_H__


/** Public functions **/
/****************************************************************
 * � appeler autour de _tout_ appel � fonctions de librairies externes
 *
 * Cela d�sactive la pr�emption et les traitants de signaux
 */
extern void marcel_extlib_protect(void);
extern void marcel_extlib_unprotect(void);


#endif /** __MARCEL_LOCKING_H__ **/
