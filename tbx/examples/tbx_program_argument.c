/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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
#include <stdio.h>
#include "tbx_program_argument.h"


#define ARG_MAX  10


int main()
{
	char  *cmd_argv[ARG_MAX];
	int    cmd_argv_size;
	char **parsed_argv;
	int    parsed_argc;

	cmd_argv_size = 0;
	cmd_argv[cmd_argv_size++] = "--tbx-opt";
	cmd_argv[cmd_argv_size++] = "tbx-opt-value";
	cmd_argv[cmd_argv_size++] = "--marcel-opt";
	cmd_argv[cmd_argv_size++] = "marcel-opt-value1";
	cmd_argv[cmd_argv_size++] = "marcel-opt-value2";
	cmd_argv[cmd_argv_size] = NULL;

	printf("parse args\n");
	if (! tbx_pa_parse(cmd_argv_size, cmd_argv, NULL))
		return 1;

	printf("getargs\n");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != cmd_argv_size)
		return 1;

	printf("free: null-module\n");
	tbx_pa_free_module_args("null-module");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != cmd_argv_size)
		return 1;

	printf("free: tbx\n");
	tbx_pa_free_module_args("tbx");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != 3)
		return 1;

	printf("free: marcel\n");
	tbx_pa_free_module_args("marcel");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != 0)
		return 1;

	tbx_pa_free_args();

	putenv("PM2_ARGS=--tbx-optbis --marcel-optbis marcel-optbis-value");
	tbx_pa_parse(cmd_argv_size, cmd_argv, "PM2_ARGS");
	cmd_argv_size += 3;
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != cmd_argv_size)
		return 1;

	printf("free: null-module\n");
	tbx_pa_free_module_args("null-module");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != cmd_argv_size)
		return 1;

	printf("free: tbx\n");
	tbx_pa_free_module_args("tbx");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != 5)
		return 1;

	printf("free: marcel\n");
	tbx_pa_free_module_args("marcel");
	if (! tbx_pa_get_args(&parsed_argc, &parsed_argv) ||
	    parsed_argc != 0)
		return 1;

	tbx_pa_free_args();
	printf("Success\n");
	return 0;
}
