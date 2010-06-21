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


#ifndef __TBX_LOG_H__
#define __TBX_LOG_H__


#include <stdarg.h>
#include "tbx_compiler.h"



#ifndef TBX_MAXLINELEN
#define TBX_MAXLINELEN 1024
#endif


/** Warning: do not forget to define level_str for new msg_level !!! **/
typedef enum {
	ERR,
	DISP,
	LOG,
	MSG_LEVEL_NUM
} tbx_msg_level_t;

#ifdef TBX_LOG_C
static char *level_str[] = {
	[DISP] = "DISP",
	[LOG] = "LOG",
	[ERR] = "ERR"
};
#endif


typedef struct {
	tbx_msg_level_t level;
	char *prefix;
} tbx_log_t;


int tbx_logger_start(int argc, char *argv[]);
void tbx_logger_stop(void);

int tbx_logger_add(tbx_log_t * logd,
		   char *prefix) __tbx_attribute_nonnull__((1));
void tbx_logger_del(tbx_log_t * logd) __tbx_attribute_nonnull__((1));
void tbx_logger_print(tbx_log_t * logd, tbx_msg_level_t level,
		      char *format, ...) __tbx_attribute_nonnull__((1, 3));
void tbx_logger_vprint(tbx_log_t * logd, tbx_msg_level_t level,
		       char *format,
		       va_list args) __tbx_attribute_nonnull__((1, 3));
void tbx_logger_set_level(tbx_log_t * logd,
			  tbx_msg_level_t newlevel)
__tbx_attribute_nonnull__((1));
tbx_msg_level_t tbx_logger_get_level(tbx_log_t *
				     logd) __tbx_attribute_nonnull__((1));

#endif /** __TBX_LOG_H__ **/
