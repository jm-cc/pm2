/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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


#include <stdlib.h>
#include "tbx.h"

#define BUF_SIZE 5

static int snprintf_test(void)
{
	int ret;
	char result[BUF_SIZE];

	ret = tbx_snprintf(result, BUF_SIZE, "%s%d", "", 0);
	if (ret != 1 || strcmp(result, "0"))
		return 0;

	ret = tbx_snprintf(result, BUF_SIZE, "%s%d", "thisisabigmessage", 0);
	if (ret != strlen("thisisabigmessage0") || 
	    strncmp(result, "thisisabigmessage0", BUF_SIZE-1))
		return 0;
	
	return 1;
}

int main()
{
	return (1 == snprintf_test()) ? EXIT_SUCCESS : EXIT_FAILURE;
}
