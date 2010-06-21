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


#ifndef __TBX_PROGRAM_ARGUMENT_H__
#define __TBX_PROGRAM_ARGUMENT_H__


#define PM2_ENVVARNAME "PM2_ARGS"


// returns arguments from command-line and environnement previously parsed by tbx
int tbx_pa_get_args(int *argc, char ***argv);

// parse environment arguments and store them with argc and argv
int tbx_pa_parse(int cmdline_argc, char *cmdline_argv[], char *env_varname);

// free memory allocated by tbx_get_args
void tbx_pa_free_args(void);

// remove 'module_name' specific arguments and free unused memory
void tbx_pa_free_module_args(char *module_name);


#endif /** __TBX_PROGRAM_ARGUMENT_H__ **/
