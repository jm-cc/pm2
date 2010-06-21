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
 * Default ouput for all pm2 modules
 *
 **/


#ifndef __TBX_DEBUG_H__
#define __TBX_DEBUG_H__


#include "tbx_log.h"
#ifdef DO_PROFILE
#include "fut_pm2.h"
#else
#define PROF_IN()  (void)0
#define PROF_OUT() (void)0
#endif


#ifdef PM2DEBUG

#ifdef TBX_C
tbx_log_t pm2events;
#else
extern tbx_log_t pm2events;
#endif /** TBX_C **/


/** logger constructors and destructors **/
#define PM2_LOG_ADD()           tbx_logger_add(&pm2events, "pm2")
#define PM2_LOG_DEL()           tbx_logger_del(&pm2events)
#define PM2_SET_LEVEL(newlevel) tbx_logger_set_level(&pm2events, newlevel)

/** LOG, DISP, ERR level macros **/
#define PM2_ERR(format, ...)    tbx_logger_print(&pm2events, ERR, format, ##__VA_ARGS__)
#define PM2_LOG(format, ...)    tbx_logger_print(&pm2events, LOG, format, ##__VA_ARGS__)
#define PM2_DISP(format, ...)   tbx_logger_print(&pm2events, DISP, format, ##__VA_ARGS__)


#else


/** logger constructors and destructors **/
#define PM2_LOG_ADD()           (void)0
#define PM2_LOG_DEL()           (void)0
#define PM2_SET_LEVEL(newlevel) (void)0

/** LOG, DISP level macros **/
#define PM2_ERR(format, ...)    fprintf(stderr, format, ##__VA_ARGS__)
#define PM2_LOG(format, ...)    (void)0
#define PM2_DISP(format, ...)   (void)0


#endif /** PM2DEBUG **/


/** PM2DEBUG & PROFILE **/
#define PM2_LOG_IN()            do { PM2_LOG("%s: -->\n", __TBX_FUNCTION__) ; PROF_IN() ; } while(0)
#define PM2_LOG_OUT()           do { PM2_LOG("%s: <--\n", __TBX_FUNCTION__) ; PROF_OUT() ; } while(0)


#endif /** __TBX_DEBUG_H__ **/
