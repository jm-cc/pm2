
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

#section functions
int marcel_init_top(const char *);
void marcel_exit_top(void);
void marcel_show_top(void);
#section marcel_functions
#ifdef MA__BUBBLES
void ma_top_add_bubble(marcel_bubble_t *b);
void ma_top_del_bubble(marcel_bubble_t *b);
#endif
