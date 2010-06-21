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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tbx_program_argument.h"
#include "tbx_types.h"


#define DEFAULT_ARGV_0      "prog"


static tbx_bool_t arg_defined = tbx_false;
static struct p_args { int argc; char **argv;} cmd_env_args;


static inline int isquote(char c);
static inline int issep(char c);
static void parse_env(char *env_varname);


// returns arguments from command-line and environnement previously parsed by tbx
int tbx_pa_get_args(int *argc, char ***argv)
{
	if (arg_defined) {
		*argc = cmd_env_args.argc;
		*argv = cmd_env_args.argv;
		return 1;
	}

	return 0;
}

// parse environment arguments and store them with argc and argv
int tbx_pa_parse(int cmdline_argc, char *cmdline_argv[], char *env_varname)
{
	int i;

	if (arg_defined)
		return 0;

	// count arguments & define the args structure
	cmd_env_args.argc = (cmdline_argc) ? cmdline_argc : 1;
	cmd_env_args.argv = (char **) malloc((cmd_env_args.argc) * sizeof(char *));
	if (!cmd_env_args.argv)
		return 0;

	// copy command-line arguments
	if (cmdline_argc == 0) {
		cmd_env_args.argv[0] = malloc(strlen(DEFAULT_ARGV_0) + 1);
		strcpy(cmd_env_args.argv[0], DEFAULT_ARGV_0);
	} else {
		for (i = 0; i < cmdline_argc; i++) {
			if (strcmp(cmdline_argv[i], "--")) {
				cmd_env_args.argv[i] = malloc(strlen(cmdline_argv[i]) + 1);
				strcpy(cmd_env_args.argv[i], cmdline_argv[i]);
			}
			else
				break;
		}
		
		cmd_env_args.argc = i;
	}

	// retrieve argument from the environment
	if (env_varname)
		parse_env(getenv(env_varname));

	// argv should be NULL terminated
	cmd_env_args.argv = (char **) realloc(cmd_env_args.argv,
					      (cmd_env_args.argc + 1) * sizeof(char *));
	cmd_env_args.argv[cmd_env_args.argc] = NULL;
	
	arg_defined = tbx_true;
	return 1;
}

// free memory allocated by tbx_get_args
void tbx_pa_free_args()
{
	if (arg_defined) {
		while (cmd_env_args.argc > 0) {
			free(cmd_env_args.argv[cmd_env_args.argc - 1]);
			cmd_env_args.argc--;
		}

		free(cmd_env_args.argv);
		arg_defined = tbx_false;
	}
}

// remove 'module_name' specific arguments and free unused memory
void tbx_pa_free_module_args(char *module_name)
{
	int i, j;
	int module_name_sz;
	tbx_bool_t is_module_option;

	if (! arg_defined)
		return;

	is_module_option = tbx_false;
	module_name_sz = strlen(module_name);
	for (i = 0, j = 0; i < cmd_env_args.argc; i++) {
		if (! strncmp(cmd_env_args.argv[i], "--", 2)) {
			if (! strncmp(cmd_env_args.argv[i] + 2, module_name, module_name_sz))
				is_module_option = tbx_true;
			else
				is_module_option = tbx_false;
		}
		
		if (tbx_false == is_module_option) {
			if (j < i) {
				cmd_env_args.argv[j] = cmd_env_args.argv[i];
				cmd_env_args.argv[i] = NULL;
			}

			j ++;
		}
		else
			free(cmd_env_args.argv[i]);
	}

	// argv should be NULL terminated
	cmd_env_args.argc = j;
	cmd_env_args.argv[cmd_env_args.argc] = NULL;
}


static inline int isquote(char c)
{
	return (c == '"' || c == '\'');
}

static inline int issep(char c)
{
	return (isquote(c) || c == ' ');
}

static void parse_env(char *env_rval)
{
	char expectedsep;
	int start, arglen;
	int i, env_rval_len;
	int argv_size;

	if (NULL == env_rval)
		return;

	argv_size = cmd_env_args.argc;
	env_rval_len = strlen(env_rval);
	arglen = 0;
	start = 0;
	expectedsep = ' ';
	for (i = 0; i <= env_rval_len; i++) {
		if (!arglen) {
			if (issep(env_rval[i]) && !isquote(expectedsep)) {
				start = i + 1;
				expectedsep = env_rval[i];
				continue;
			}
		} else {
			if (expectedsep == env_rval[i] || '\0' == env_rval[i]) {
				if (cmd_env_args.argc == argv_size) {
					argv_size += 3;
					cmd_env_args.argv = (char **) realloc(cmd_env_args.argv,
									      argv_size * sizeof(char *));
				}
				
				cmd_env_args.argv[cmd_env_args.argc] = (char *) malloc(arglen + 1);
				strncpy(cmd_env_args.argv[cmd_env_args.argc], env_rval + start, arglen);
				cmd_env_args.argv[cmd_env_args.argc][arglen] = 0;
				cmd_env_args.argc++;
				expectedsep = ' ';
				start = i + 1;
				arglen = 0;
				continue;
			}
		}

		if (expectedsep != env_rval[i])
			arglen++;
	}
}
