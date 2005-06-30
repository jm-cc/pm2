
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


#ifndef FKT_PM2_IS_DEF
#define FKT_PM2_IS_DEF

extern int fkt_record(char *file, unsigned int initmask, int powpages, int dma);
extern void fkt_stop(void);
extern void fkt_keychange(int how, unsigned int keymask);

#endif
