
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


/* included by xpm2.c */
#ifndef __lire_console_h__
#define __lire_console_h__

#include "structures.h"

void       extract_word (char *buf, char nom[128], int *i);
p_host     extract_conf (char *buf, p_host previous_hosts);
p_pvm_task extract_ps   (char *buf, p_host ph, p_pvm_task previous_pvm_tasks);
p_thread   extract_th   (char *buf, p_pvm_task ppt, p_thread previous_threads);

#endif /* __lire_console_h__ */

