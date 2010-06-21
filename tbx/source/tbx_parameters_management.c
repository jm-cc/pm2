/*! \file tbx_parameters_management.c
 *  \brief TBX command line management routines
 *
 *  This file implements TBX command line construction functions, 
 *  command-string, argument, environment building functions
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
 * tbx_parameter_management.c
 * :::::::::::::::::::::::::://///////////////////////////////////////////////
 */

#include "tbx.h"


/*
 *  Functions
 * ___________________________________________________________________________
 */


/*
 * Initialization
 * --------------
 */
void tbx_parameter_manager_init(void)
{
	PM2_LOG_IN();
	PM2_LOG_OUT();
}

void tbx_parameter_manager_exit(void)
{
	PM2_LOG_IN();
	PM2_LOG_OUT();
}

// ... Argument option ................................................. //
p_tbx_argument_option_t
tbx_argument_option_init_to_cstring_ext(const char *option,
					const char separator,
					const char *value)
{
	p_tbx_argument_option_t object = NULL;

	PM2_LOG_IN();
	object = TBX_CALLOC(1, sizeof(tbx_argument_option_t));
	CTRL_ALLOC(object);

	object->option = tbx_string_init_to_cstring(option);
	object->separator = separator;

	if (value) {
		object->value = tbx_string_init_to_cstring(value);
	} else {
		object->value = NULL;
	}
	PM2_LOG_OUT();

	return object;
}

p_tbx_argument_option_t
tbx_argument_option_init_to_cstring(const char *option)
{
	p_tbx_argument_option_t object = NULL;

	PM2_LOG_IN();
	object = tbx_argument_option_init_to_cstring_ext(option,
							 '\0', NULL);
	PM2_LOG_OUT();

	return object;
}

p_tbx_argument_option_t
tbx_argument_option_init_ext(p_tbx_string_t option,
			     char separator, p_tbx_string_t value)
{
	p_tbx_argument_option_t object = NULL;

	PM2_LOG_IN();
	object = TBX_CALLOC(1, sizeof(tbx_argument_option_t));
	CTRL_ALLOC(object);

	object->option = tbx_string_dup(option);
	object->separator = separator;
	if (value) {
		object->value = tbx_string_dup(value);
	}
	PM2_LOG_OUT();

	return object;
}

p_tbx_argument_option_t tbx_argument_option_init(p_tbx_string_t option)
{
	p_tbx_argument_option_t object = NULL;

	PM2_LOG_IN();
	object = tbx_argument_option_init_ext(option, '\0', NULL);
	PM2_LOG_OUT();

	return object;
}

p_tbx_argument_option_t
tbx_argument_option_dup(p_tbx_argument_option_t arg)
{
	p_tbx_argument_option_t object = NULL;

	PM2_LOG_IN();
	object =
	    tbx_argument_option_init_ext(arg->option, arg->separator,
					 arg->value);
	PM2_LOG_OUT();

	return object;
}

void tbx_argument_option_free(p_tbx_argument_option_t arg)
{
	PM2_LOG_IN();
	tbx_string_free(arg->option);
	arg->option = NULL;
	arg->separator = '\0';

	if (arg->value) {
		tbx_string_free(arg->value);
		arg->value = NULL;
	}
	TBX_FREE(arg);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_argument_option_to_string(p_tbx_argument_option_t arg)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init_to_string(arg->option);

	if (arg->separator) {
		tbx_string_append_char(string, arg->separator);
	}

	if (arg->value) {
		tbx_string_append_string(string, arg->value);
	}
	PM2_LOG_OUT();

	return string;
}

// ... Arguments ....................................................... //
p_tbx_arguments_t tbx_arguments_init(void)
{
	p_tbx_arguments_t args = NULL;

	PM2_LOG_IN();
	args = TBX_CALLOC(1, sizeof(tbx_arguments_t));
	args->slist = tbx_slist_nil();
	PM2_LOG_OUT();

	return args;
}

static
void *__tbx_arguments_dup_func(void *src_object)
{
	void *dst_object = NULL;

	PM2_LOG_IN();
	dst_object = tbx_argument_option_dup(src_object);
	PM2_LOG_OUT();

	return dst_object;
}

p_tbx_arguments_t tbx_arguments_dup(p_tbx_arguments_t src_args)
{
	p_tbx_arguments_t dst_args = NULL;
	p_tbx_slist_t src_slist = NULL;

	PM2_LOG_IN();
	dst_args = TBX_CALLOC(1, sizeof(tbx_arguments_t));
	src_slist = src_args->slist;
	dst_args->slist =
	    tbx_slist_dup_ext(src_slist, __tbx_arguments_dup_func);
	PM2_LOG_OUT();

	return dst_args;
}

void
tbx_arguments_append_arguments(p_tbx_arguments_t dst_args,
			       p_tbx_arguments_t src_args)
{
	p_tbx_slist_t dst_slist = NULL;
	p_tbx_slist_t src_slist = NULL;

	PM2_LOG_IN();
	dst_slist = dst_args->slist;
	src_slist = src_args->slist;
	tbx_slist_merge_after_ext(dst_slist, src_slist,
				  __tbx_arguments_dup_func);
	PM2_LOG_OUT();
}

void tbx_arguments_free(p_tbx_arguments_t args)
{
	PM2_LOG_IN();
	while (!tbx_slist_is_nil(args->slist)) {
		p_tbx_argument_option_t arg = NULL;

		arg = tbx_slist_extract(args->slist);
		tbx_argument_option_free(arg);
	}

	tbx_slist_free(args->slist);
	args->slist = NULL;

	TBX_FREE(args);
	PM2_LOG_OUT();
}

void
tbx_arguments_append_option(p_tbx_arguments_t args,
			    p_tbx_argument_option_t arg)
{
	PM2_LOG_IN();
	tbx_slist_append(args->slist, arg);
	PM2_LOG_OUT();
}

void
tbx_arguments_append_string_ext(p_tbx_arguments_t args,
				p_tbx_string_t option,
				const char separator, p_tbx_string_t value)
{
	p_tbx_argument_option_t arg = NULL;

	PM2_LOG_IN();
	arg = tbx_argument_option_init_ext(option, separator, value);
	tbx_arguments_append_option(args, arg);
	PM2_LOG_OUT();
}

void
tbx_arguments_append_string(p_tbx_arguments_t args, p_tbx_string_t option)
{
	PM2_LOG_IN();
	tbx_arguments_append_string_ext(args, option, '\0', NULL);
	PM2_LOG_OUT();
}

void
tbx_arguments_append_cstring_ext(p_tbx_arguments_t args,
				 const char *option,
				 const char separator, const char *value)
{
	p_tbx_argument_option_t arg = NULL;

	PM2_LOG_IN();
	arg =
	    tbx_argument_option_init_to_cstring_ext(option, separator,
						    value);
	tbx_arguments_append_option(args, arg);
	PM2_LOG_OUT();
}

void
tbx_arguments_append_cstring(p_tbx_arguments_t args, const char *option)
{
	PM2_LOG_IN();
	tbx_arguments_append_cstring_ext(args, option, '\0', NULL);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_arguments_to_string(p_tbx_arguments_t args, int IFS)
{
	p_tbx_string_t string = NULL;
	p_tbx_slist_t slist = NULL;

	PM2_LOG_IN();
	slist = args->slist;
	if (tbx_slist_is_nil(slist)) {
		string = tbx_string_init();
	} else {
		p_tbx_argument_option_t arg = NULL;
		p_tbx_string_t temp_string = NULL;

		tbx_slist_ref_to_head(slist);
		arg = tbx_slist_ref_get(slist);
		temp_string = tbx_argument_option_to_string(arg);
		string = tbx_string_init_to_string_and_free(temp_string);

		while (tbx_slist_ref_forward(slist)) {
			tbx_string_append_char(string, IFS);

			arg = tbx_slist_ref_get(slist);
			temp_string = tbx_argument_option_to_string(arg);
			tbx_string_append_string_and_free(string,
							  temp_string);
		}
	}
	PM2_LOG_OUT();

	return string;
}


// ... Argument set .................................................... //
p_tbx_argument_set_t tbx_argument_set_build(p_tbx_arguments_t args)
{
	p_tbx_argument_set_t arg_set = NULL;

	PM2_LOG_IN();
	arg_set = TBX_CALLOC(1, sizeof(tbx_argument_set_t));
	CTRL_ALLOC(arg_set);

	arg_set->argc = tbx_slist_get_length(args->slist) + 1;
	arg_set->argv =
	    TBX_CALLOC((size_t) arg_set->argc + 1, sizeof(char *));

	arg_set->argv[0] = NULL;

	if (!tbx_slist_is_nil(args->slist)) {
		p_tbx_slist_t slist = args->slist;
		int idx = 1;

		tbx_slist_ref_to_head(slist);
		do {
			p_tbx_argument_option_t arg = NULL;
			p_tbx_string_t arg_string = NULL;

			arg = tbx_slist_ref_get(slist);
			arg_string = tbx_argument_option_to_string(arg);
			arg_set->argv[idx++] =
			    tbx_string_to_cstring_and_free(arg_string);
		}
		while (tbx_slist_ref_forward(slist));

		arg_set->argv[idx] = NULL;
	}
	PM2_LOG_OUT();

	return arg_set;
}

void tbx_argument_set_free(p_tbx_argument_set_t args)
{
	PM2_LOG_IN();
	while (args->argc--) {
		if (args->argv[args->argc]) {
			TBX_FREE(args->argv[args->argc]);
			args->argv[args->argc] = NULL;
		}
	}

	args->argc = 0;

	TBX_FREE(args->argv);
	args->argv = NULL;

	TBX_FREE(args);
	PM2_LOG_OUT();
}

p_tbx_argument_set_t
tbx_argument_set_build_ext(p_tbx_arguments_t args, p_tbx_string_t command)
{
	p_tbx_argument_set_t arg_set = NULL;

	PM2_LOG_IN();
	arg_set = tbx_argument_set_build(args);
	arg_set->argv[0] = tbx_string_to_cstring(command);
	PM2_LOG_OUT();

	return arg_set;
}

p_tbx_argument_set_t
tbx_argument_set_build_ext_c_command(const p_tbx_arguments_t args,
				     const char *command)
{
	p_tbx_argument_set_t arg_set = NULL;

	PM2_LOG_IN();
	arg_set = tbx_argument_set_build(args);
	arg_set->argv[0] = tbx_strdup(command);
	PM2_LOG_OUT();

	return arg_set;
}


// ... Environment variable ............................................ //
p_tbx_environment_variable_t
tbx_environment_variable_init(p_tbx_string_t name, p_tbx_string_t value)
{
	p_tbx_environment_variable_t variable = NULL;

	PM2_LOG_IN();
	variable = TBX_CALLOC(1, sizeof(tbx_environment_variable_t));
	CTRL_ALLOC(variable);

	variable->name = tbx_string_init_to_string(name);
	if (value) {
		variable->value = tbx_string_init_to_string(value);
	} else {
		variable->value = tbx_string_init();
	}
	PM2_LOG_OUT();

	return variable;
}

p_tbx_environment_variable_t
tbx_environment_variable_init_to_cstrings(const char *name,
					  const char *value)
{
	p_tbx_environment_variable_t variable = NULL;

	PM2_LOG_IN();
	variable = TBX_CALLOC(1, sizeof(tbx_environment_variable_t));
	CTRL_ALLOC(variable);

	variable->name = tbx_string_init_to_cstring(name);
	if (value) {
		variable->value = tbx_string_init_to_cstring(value);
	} else {
		variable->value = tbx_string_init();
	}
	PM2_LOG_OUT();

	return variable;
}

p_tbx_environment_variable_t
tbx_environment_variable_to_variable(const char *name)
{
	p_tbx_environment_variable_t variable = NULL;
	char *value = NULL;

	PM2_LOG_IN();
	variable = TBX_CALLOC(1, sizeof(tbx_environment_variable_t));
	CTRL_ALLOC(variable);

	variable->name = tbx_string_init_to_cstring(name);
	value = getenv(name);

	if (value) {
		variable->value = tbx_string_init_to_cstring(value);
	} else {
		variable->value = tbx_string_init();
	}
	PM2_LOG_OUT();

	return variable;
}

void
tbx_environment_variable_append_cstring(p_tbx_environment_variable_t var,
					const char sep, const char *data)
{
	PM2_LOG_IN();
	if (sep && tbx_string_length(var->value)) {
		tbx_string_append_char(var->value, sep);
	}

	tbx_string_append_cstring(var->value, data);
	PM2_LOG_OUT();
}

void tbx_environment_variable_free(p_tbx_environment_variable_t variable)
{
	PM2_LOG_IN();
	tbx_string_free(variable->name);
	variable->name = NULL;

	if (variable->value) {
		tbx_string_free(variable->value);
		variable->value = NULL;
	}
	PM2_LOG_OUT();
}

p_tbx_string_t
tbx_environment_variable_to_string(p_tbx_environment_variable_t variable)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init_to_string(variable->name);
	tbx_string_append_char(string, '=');
	{
		p_tbx_string_t value_string = NULL;

		value_string = tbx_string_double_quote(variable->value);
		tbx_string_append_string_and_free(string, value_string);
	}

	PM2_LOG_OUT();

	return string;
}

void
tbx_environment_variable_set_to_string(p_tbx_environment_variable_t
				       variable, p_tbx_string_t value)
{
	PM2_LOG_IN();
	tbx_string_set_to_string(variable->value, value);
	PM2_LOG_OUT();
}

void
tbx_environment_variable_set_to_cstring(p_tbx_environment_variable_t
					variable, const char *value)
{
	PM2_LOG_IN();
	tbx_string_set_to_cstring(variable->value, value);
	PM2_LOG_OUT();
}


// ... Environments .................................................... //
p_tbx_environment_t tbx_environment_init(void)
{
	p_tbx_environment_t env = NULL;

	PM2_LOG_IN();
	env = TBX_CALLOC(1, sizeof(tbx_environment_t));
	CTRL_ALLOC(env);

	env->slist = tbx_slist_nil();
	PM2_LOG("env->slist %p", env->slist);
	env->htable = tbx_htable_empty_table();
	PM2_LOG_OUT();

	return env;
}

void tbx_environment_free(p_tbx_environment_t env)
{
	PM2_LOG_IN();
	while (!tbx_slist_is_nil(env->slist)) {
		p_tbx_environment_variable_t var = NULL;

		var = tbx_slist_extract(env->slist);
		tbx_environment_variable_free(var);
	}

	tbx_slist_free(env->slist);
	env->slist = NULL;
	tbx_htable_cleanup_and_free(env->htable);
	env->htable = NULL;

	TBX_FREE(env);
	PM2_LOG_OUT();
}

void
tbx_environment_append_variable(p_tbx_environment_t env,
				p_tbx_environment_variable_t var)
{
	char *name = NULL;

	PM2_LOG_IN();
	name = tbx_string_to_cstring(var->name);

	if (tbx_htable_get(env->htable, name))
		TBX_FAILURE("argument already exist");

	tbx_htable_add(env->htable, name, var);
	tbx_slist_append(env->slist, var);

	TBX_FREE(name);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_environment_to_string(p_tbx_environment_t env)
{
	p_tbx_string_t string = NULL;
	p_tbx_slist_t slist = NULL;

	PM2_LOG_IN();
	slist = env->slist;

	if (tbx_slist_is_nil(slist)) {
		string = tbx_string_init();
	} else {
		p_tbx_environment_variable_t var = NULL;
		p_tbx_string_t temp_string = NULL;

		tbx_slist_ref_to_head(slist);
		var = tbx_slist_ref_get(slist);
		temp_string = tbx_environment_variable_to_string(var);
		string = tbx_string_init_to_string_and_free(temp_string);

		while (tbx_slist_ref_forward(slist)) {
			tbx_string_append_char(string, ' ');

			var = tbx_slist_ref_get(slist);
			temp_string =
			    tbx_environment_variable_to_string(var);
			tbx_string_append_string_and_free(string,
							  temp_string);
		}
	}
	PM2_LOG_OUT();

	return string;
}


// ... Command ......................................................... //
p_tbx_command_t tbx_command_init(p_tbx_string_t command_name)
{
	p_tbx_command_t command = NULL;

	PM2_LOG_IN();
	command = TBX_CALLOC(1, sizeof(tbx_command_t));
	CTRL_ALLOC(command);

	command->environment = tbx_environment_init();
	command->path = tbx_string_init_to_string(command_name);
	command->arguments = tbx_arguments_init();

	{
		p_tbx_string_t name_string = NULL;
		p_tbx_slist_t slist = NULL;

		name_string =
		    tbx_string_extract_name_from_pathname(command->path);
		slist = tbx_string_split_and_free(name_string, NULL);
		command->name = tbx_slist_extract(slist);

		while (!tbx_slist_is_nil(slist)) {
			p_tbx_string_t arg_string = NULL;

			arg_string = tbx_slist_extract(slist);
			tbx_arguments_append_string_ext(command->arguments,
							arg_string, 0,
							NULL);
		}

		tbx_slist_free(slist);
	}
	PM2_LOG_OUT();

	return command;
}

p_tbx_command_t tbx_command_init_to_cstring(const char *command_name)
{
	p_tbx_command_t command = NULL;
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init_to_cstring(command_name);
	command = tbx_command_init(string);
	tbx_string_free(string);
	PM2_LOG_OUT();

	return command;
}

void tbx_command_free(p_tbx_command_t command)
{
	PM2_LOG_IN();
	tbx_environment_free(command->environment);
	command->environment = NULL;

	tbx_string_free(command->path);
	command->path = NULL;

	tbx_string_free(command->name);
	command->name = NULL;

	tbx_arguments_free(command->arguments);
	command->arguments = NULL;

	TBX_FREE(command);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_command_to_string(p_tbx_command_t command)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();

	if (!tbx_slist_is_nil(command->environment->slist)) {
		p_tbx_string_t env_string = NULL;

		env_string =
		    tbx_environment_to_string(command->environment);
		tbx_string_append_string_and_free(string, env_string);
		tbx_string_append_char(string, (int) ' ');
	}

	tbx_string_append_string(string, command->path);
	tbx_string_append_string(string, command->name);

	if (!tbx_slist_is_nil(command->arguments->slist)) {
		p_tbx_string_t arg_string = NULL;

		tbx_string_append_char(string, ' ');

		arg_string =
		    tbx_arguments_to_string(command->arguments, (int) ' ');
		tbx_string_append_string_and_free(string, arg_string);
	}
	PM2_LOG_OUT();

	return string;
}

p_tbx_argument_set_t tbx_command_to_argument_set(p_tbx_command_t command)
{
	p_tbx_argument_set_t arg_set = NULL;
	p_tbx_string_t path_name = NULL;

	PM2_LOG_IN();
	path_name = tbx_string_init_to_string(command->path);
	tbx_string_append_string(path_name, command->name);

	arg_set =
	    tbx_argument_set_build_ext(command->arguments, path_name);
	tbx_string_free(path_name);
	PM2_LOG_OUT();

	return arg_set;
}
