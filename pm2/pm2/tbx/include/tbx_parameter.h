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

/*
 * Tbx_parameter.h : argument list building
 * ===========-----------------------------
 */

#ifndef TBX_PARAMETER_H
#define TBX_PARAMETER_H

/*
 * Data structures 
 * ---------------
 */

typedef struct s_tbx_argument_option
{
  p_tbx_string_t option;
  char           separator;
  p_tbx_string_t value;
} tbx_argument_option_t;

typedef struct s_tbx_arguments
{
  p_tbx_slist_t slist;
} tbx_arguments_t;

typedef struct s_tbx_argument_set
{
  int    argc;
  char **argv;
} tbx_argument_set_t;

typedef struct s_tbx_environment_variable
{
  p_tbx_string_t name;
  p_tbx_string_t value;
} tbx_environment_variable_t;

typedef struct s_tbx_environment
{
  p_tbx_slist_t  slist;
  p_tbx_htable_t htable;
} tbx_environment_t;

typedef struct s_tbx_command
{
  p_tbx_environment_t environment;
  p_tbx_string_t      path;
  p_tbx_string_t      name;
  p_tbx_arguments_t   arguments;
} tbx_command_t;

#endif /* TBX_PARAMETER_H */
