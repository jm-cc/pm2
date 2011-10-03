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


#undef NDEBUG
#include <assert.h>
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
	assert(tbx_pa_parse(cmd_argv_size, cmd_argv, NULL));

	printf("getargs\n");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == cmd_argv_size);

	printf("free: null-module\n");
	tbx_pa_free_module_args("null-module");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == cmd_argv_size);

	printf("free: tbx\n");
	tbx_pa_free_module_args("tbx");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == 3);

	printf("free: marcel\n");
	tbx_pa_free_module_args("marcel");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == 0);

	tbx_pa_free_args();

	putenv("PM2_ARGS=--tbx-optbis --marcel-optbis marcel-optbis-value");
	tbx_pa_parse(cmd_argv_size, cmd_argv, "PM2_ARGS");
	cmd_argv_size += 3;
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == cmd_argv_size);

	printf("free: null-module\n");
	tbx_pa_free_module_args("null-module");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == cmd_argv_size);

	printf("free: tbx\n");
	tbx_pa_free_module_args("tbx");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == 5);

	printf("free: marcel\n");
	tbx_pa_free_module_args("marcel");
	assert(tbx_pa_get_args(&parsed_argc, &parsed_argv) &&
	       parsed_argc == 0);

	printf("copy unparsed args\n");
	tbx_pa_copy_args(&cmd_argv_size, cmd_argv);
	assert(cmd_argv_size == 0 && parsed_argc == cmd_argv_size);

	tbx_pa_free_args();
	return 0;
}
