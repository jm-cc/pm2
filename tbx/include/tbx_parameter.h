/*! \file tbx_parameter.h
 *  \brief TBX command line management data structures
 *
 *  This file provides TBX command-string, argument, 
 *  environment building data structures
 * 
 */

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

/** \defgroup param_interface command line args parsing interface
 *
 * This is the command line args parsing interface
 *
 * @{
 */

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_argument_option {
	p_tbx_string_t option;
	char separator;
	p_tbx_string_t value;
} tbx_argument_option_t;

typedef struct s_tbx_arguments {
	p_tbx_slist_t slist;
} tbx_arguments_t;

typedef struct s_tbx_argument_set {
	int argc;
	char **argv;
} tbx_argument_set_t;

typedef struct s_tbx_environment_variable {
	p_tbx_string_t name;
	p_tbx_string_t value;
} tbx_environment_variable_t;

typedef struct s_tbx_environment {
	p_tbx_slist_t slist;
	p_tbx_htable_t htable;
} tbx_environment_t;

typedef struct s_tbx_command {
	p_tbx_environment_t environment;
	p_tbx_string_t path;
	p_tbx_string_t name;
	p_tbx_arguments_t arguments;
} tbx_command_t;


/*
 * Argument iterator 
 * -----------------
 *
 * Note: this feature uses GCC nested functions
 * ====
 */

#define __TBX_ARGIT_MORE_ARGS(ARGC, ARGV) \
  tbx_bool_t tbx_argit_more_args(void)    \
    {                                     \
      return ((ARGC) != 0);               \
    }

#define __TBX_ARGIT_ARG_CSTR(ARGC, ARGV, USAGE) \
  char *tbx_argit_arg_cstr(void)                \
    {                                           \
      if (!tbx_argit_more_args())               \
        USAGE();                                \
      return tbx_strdup(*(ARGV));               \
    }

#define __TBX_ARGIT_ARG_EQUALS(ARGC, ARGV, USAGE) \
  tbx_bool_t tbx_argit_arg_equals(char *arg)      \
    {                                             \
      if (!tbx_argit_more_args())                 \
        USAGE();                                  \
      return !strcmp(*(ARGV), arg);               \
    }

#define __TBX_ARGIT_NEXT_ARG(ARGC, ARGV) \
  tbx_bool_t tbx_argit_next_arg(void)    \
    {                                    \
      (ARGC)--;                          \
      (ARGV)++;                          \
      return tbx_argit_more_args();      \
    }

#define __TBX_ARGIT_OPT_CSTR(ARGC, ARGV) \
  char *tbx_argit_opt_cstr(void)         \
    {                                    \
      return tbx_argit_arg_cstr();       \
    }

#define __TBX_ARGIT_OPT_EQUALS(ARGC, ARGV)   \
  tbx_bool_t tbx_argit_opt_equals(char *arg) \
    {                                        \
      return tbx_argit_arg_equals(arg);      \
    }

#define __TBX_ARGIT_VOPT_EQUALS(ARGC, ARGV, USAGE) \
  tbx_bool_t tbx_argit_vopt_equals(char *arg)      \
    {                                              \
      int  _len = strlen(arg);                     \
      char _arg[_len + 2];                         \
      strcpy(_arg, arg);                           \
      strcat(_arg, "=");                           \
      if (!tbx_argit_more_args())                  \
        USAGE();                                   \
      return !strncmp(*(ARGV), _arg, _len + 1);    \
    }

#define __TBX_ARGIT_VOPT_VALUE_CSTR(ARGC, ARGV, USAGE) \
  char *tbx_argit_vopt_value_cstr(void)                \
    {                                                  \
      char *ptr = NULL;                                \
      ptr = strchr(*(ARGV), '=');                      \
      if (!ptr)                                        \
        USAGE();                                       \
      ptr++;                                           \
      return tbx_strdup(ptr);                          \
    }

#define __TBX_ARGIT_MORE_OPTS(ARGC, ARGV) \
  tbx_bool_t tbx_argit_more_opts(void)    \
    {                                     \
      if (!tbx_argit_more_args())         \
	return tbx_false;                 \
                                          \
      if ((*(ARGV))[0] != '-')            \
	return tbx_false;                 \
                                          \
      if (tbx_argit_arg_equals("--"))     \
	{                                 \
	  tbx_argit_next_arg();           \
	  return tbx_false;               \
	}                                 \
                                          \
      return tbx_true;                    \
    }

#define __TBX_ARGIT_NEXT_OPT(ARGC, ARGV) \
  tbx_bool_t tbx_argit_next_opt(void)    \
    {                                    \
      tbx_argit_next_arg();              \
      return tbx_argit_more_opts();      \
    }

#define __TBX_ARGIT_VALUE_CSTR(ARGC, ARGV, USAGE) \
  char *tbx_argit_value_cstr(void)                \
    {                                             \
      tbx_argit_next_arg();                       \
                                                  \
      if (argc)                                   \
	{                                         \
	  return tbx_argit_arg_cstr();            \
	}                                         \
      else                                        \
	USAGE();                                  \
    }

#define __TBX_ARGIT_INVALID_ARG(ARGC, ARGV, USAGE)        \
  void tbx_argit_invalid_arg(void)                        \
    {                                                     \
      fprintf(stderr, "invalid argument: %s\n", *(ARGV)); \
      USAGE();                                            \
    }

#define __TBX_ARGIT_INIT(ARGC, ARGV) \
  tbx_bool_t tbx_argit_init(void)    \
    {                                \
      tbx_argit_next_arg();          \
      return tbx_argit_more_args();  \
    }

#define tbx_arg_iterator(ARGC, ARGV, USAGE)    \
__TBX_ARGIT_MORE_ARGS(ARGC, ARGV)              \
__TBX_ARGIT_ARG_CSTR(ARGC, ARGV, USAGE)        \
__TBX_ARGIT_ARG_EQUALS(ARGC, ARGV, USAGE)      \
__TBX_ARGIT_NEXT_ARG(ARGC, ARGV)               \
__TBX_ARGIT_OPT_CSTR(ARGC, ARGV)               \
__TBX_ARGIT_OPT_EQUALS(ARGC, ARGV)             \
__TBX_ARGIT_VOPT_EQUALS(ARGC, ARGV, USAGE)     \
__TBX_ARGIT_VOPT_VALUE_CSTR(ARGC, ARGV, USAGE) \
__TBX_ARGIT_MORE_OPTS(ARGC, ARGV)              \
__TBX_ARGIT_NEXT_OPT(ARGC, ARGV)               \
__TBX_ARGIT_VALUE_CSTR(ARGC, ARGV, USAGE)      \
__TBX_ARGIT_INVALID_ARG(ARGC, ARGV, USAGE)     \
__TBX_ARGIT_INIT(ARGC, ARGV)

/* @} */

#endif				/* TBX_PARAMETER_H */
