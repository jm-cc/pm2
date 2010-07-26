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

/**
 * Tiny event logger
 *
 * The default output is stderr.
 * It is not thread safe, you must protect tbx_logger_*() calls !
 *
 **/


#define  TBX_LOG_C
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include "tbx_log.h"
#include "tbx_snprintf.h"
#include "tbx_hooks.h"


#define DEFAULT_BUFFER_SIZE 1000	// arbitrary value --> may raise SIGSEGV


static int output_fd = -1;
static int instance_num = 0;
static int reg_logd_size = 0;
static tbx_log_t **reg_logd = NULL;

static void tbx_logger_show_registered(void);
static void tbx_logger_set_level_for_all(tbx_msg_level_t newlevel);
static tbx_log_t *tbx_logger_get_from_prefix(char *prefix);


/** module specific operations **/
int tbx_logger_add(tbx_log_t * logd, char *prefix)
{
	if (!logd || !prefix)
		return -1;

	if (reg_logd_size == instance_num) {
		reg_logd =
		    (tbx_log_t **) realloc(reg_logd,
					   (reg_logd_size +
					    10) * sizeof(tbx_log_t *));
		if (NULL == reg_logd) {
			perror("alloc");
			return -1;
		}

		reg_logd_size += 10;
	}

	logd->level = ERR;
	logd->prefix = strdup(prefix);

	reg_logd[instance_num] = logd;
	instance_num++;
	return 0;
}

void tbx_logger_del(tbx_log_t * logd)
{
	int i;

	if (!logd)
		return;

	if (instance_num > 0) {
		free(logd->prefix);
		if (instance_num == 1) {
			free(reg_logd);
			reg_logd = NULL;
		} else {
			/** removes logd from the array **/
			i = 0;
			while (i < instance_num && logd != reg_logd[i])
				i++;
			reg_logd[i] = reg_logd[instance_num - 1];
		}

		instance_num--;
	}
}

void tbx_logger_print(tbx_log_t * logd, tbx_msg_level_t level,
		      char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	tbx_logger_vprint(logd, level, format, ap);
	va_end(ap);
}

void tbx_logger_vprint(tbx_log_t * logd, tbx_msg_level_t level,
		       char *format, va_list args)
{
	int buffer_size;
	char buffer[TBX_MAXLINELEN];

	if (!logd || !format || !instance_num)
		return;

	if (level <= logd->level && -1 != output_fd) {
		buffer_size =
		    tbx_snprintf(buffer, TBX_MAXLINELEN, "%s[%s]: ",
				 logd->prefix, level_str[level]);
		buffer_size +=
		    tbx_vsnprintf(buffer + buffer_size,
				  TBX_MAXLINELEN - buffer_size, format,
				  args);
		tbx_write(output_fd, buffer, buffer_size);
	}
}

void tbx_logger_set_level(tbx_log_t * logd, tbx_msg_level_t newlevel)
{
	if (logd)
		logd->level = newlevel;
}

tbx_msg_level_t tbx_logger_get_level(tbx_log_t * logd)
{
	return ((logd && instance_num > 0) ? logd->level : ERR);
}

int tbx_logger_start(int argc, char *argv[])
{
	tbx_log_t *logd;
	tbx_msg_level_t debug_level = ERR;
	char *debug_module_list = NULL;
	char *debug_module;
	char *strtok_buffer[DEFAULT_BUFFER_SIZE];
	int fd;
	char *debug_output = NULL;

	if (!argv || -1 != output_fd)
		return -1;

	while (argc > 1) {
		if (!strncmp(argv[argc - 1], "--debug:", 8))
			debug_module_list = argv[argc - 1] + 8;

		if (!strncmp(argv[argc - 1], "--debug-level:", 14))
			debug_level = (tbx_msg_level_t)strtol(argv[argc - 1] + 14, (char **) NULL, 10);

		if (!strncmp(argv[argc - 1], "--debug-output:", 15))
			debug_output = argv[argc - 1] + 15;

		argc--;
	}

	/* check input validity */
	if (NULL == debug_module_list)
		return 0;

	if (debug_level >= MSG_LEVEL_NUM) {
		fprintf(stderr, "Debug level is unknown\n");
		exit(1);
	}

	/* specials --debug: option values */
	if (!strcmp(debug_module_list, "register")) {
		tbx_logger_show_registered();
		exit(1);
	}

	/* init output descriptor */
	output_fd = STDERR_FILENO;
	if (NULL != debug_output) {
		if (-1 != (fd = tbx_open(debug_output, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR)))
			output_fd = fd;
	}

	if (!strcmp(debug_module_list, "all")) {
		tbx_logger_set_level_for_all(debug_level);
		return 0;
	}

	/* set debug_level level for module in the module list */
	debug_module = strtok_r(debug_module_list, ",", (char **) &strtok_buffer);
	while (debug_module) {
		if ((logd = tbx_logger_get_from_prefix(debug_module)))
			logd->level = debug_level;

		debug_module = strtok_r(NULL, ",", (char **) &strtok_buffer);
	}

	return 0;
}

void tbx_logger_stop()
{
	int i;

	if (-1 == output_fd)
		return;

	if (output_fd != STDERR_FILENO) {
		tbx_close(output_fd);
		output_fd = -1;
	}

	if (reg_logd) {
		for (i = 0; i < instance_num; i++)
			tbx_logger_del(reg_logd[i]);
	}
}



/** assume that logd have an unique prefix **/
static tbx_log_t *tbx_logger_get_from_prefix(char *prefix)
{
	int i;

	for (i = 0; i < instance_num; i++) {
		if (!strcmp(prefix, reg_logd[i]->prefix))
			return reg_logd[i];
	}

	return NULL;
}

static void tbx_logger_set_level_for_all(tbx_msg_level_t newlevel)
{
	int i;

	for (i = 0; i < instance_num; i++)
		reg_logd[i]->level = newlevel;
}

static void tbx_logger_show_registered(void)
{
	int i;

	fprintf(stderr,"List of registered debugging modules:\n");

	for (i = 0; i < instance_num; i++)
		fprintf(stderr, "registered module (level): %s (%d)\n",
			reg_logd[i]->prefix, (int)reg_logd[i]->level);
}
