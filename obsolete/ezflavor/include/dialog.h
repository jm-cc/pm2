
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

#ifndef DIALOG_IS_DEF
#define DIALOG_IS_DEF

typedef void (*dialog_func_t)(gpointer);

void dialog_prompt(char *question,
		   char *choice1, dialog_func_t func1, gpointer data1,
		   char *choice2, dialog_func_t func2, gpointer data2,
		   char *choice3, dialog_func_t func3, gpointer data3);

#endif
