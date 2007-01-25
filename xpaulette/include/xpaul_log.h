/*
 * XPaulette
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifndef XPAUL_LOG_H
#define XPAUL_LOG_H

#define XPAUL_DISPF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ## __VA_ARGS__)
#define XPAUL_DISP_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define XPAUL_DISP_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define XPAUL_DISP_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define XPAUL_DISP_CHAR(val)		fprintf(stderr, "%s, %c", __TBX_FUNCTION__ , (char)(val))
#define XPAUL_DISP_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define XPAUL_DISP_STR(str, str2)		fprintf(stderr, "%s, " str ": %s\n, __TBX_FUNCTION__" , (char *)(str2))

#ifdef CONFIG_LOG
#define XPAUL_LOGF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ##__VA_ARGS__)
#define XPAUL_LOG_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define XPAUL_LOG_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define XPAUL_LOG_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define XPAUL_LOG_CHAR(val)		fprintf(stderr, "%s, %c" ,, __TBX_FUNCTION__ (char)(val))
#define XPAUL_LOG_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define XPAUL_LOG_STR(str, str2)		fprintf(stderr,"%s, "  str ": %s\n", __TBX_FUNCTION__ , (char *)(str2))
#else
#define XPAUL_LOGF(str, ...)     (void)(0)
#define XPAUL_LOG_IN()           (void)(0)
#define XPAUL_LOG_OUT()          (void)(0)
#define XPAUL_LOG_CHAR(val)      (void)(0)
#define XPAUL_LOG_VAL(str, val)  (void)(0)
#define XPAUL_LOG_PTR(str, ptr)  (void)(0)
#define XPAUL_LOG_STR(str, str2) (void)(0)
#endif

#ifdef CONFIG_TRACE
#define XPAUL_TRACEF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ## __VA_ARGS__)
#define XPAUL_TRACE_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define XPAUL_TRACE_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define XPAUL_TRACE_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define XPAUL_TRACE_CHAR(val)		fprintf(stderr, "%s, %c" , (char)(val))
#define XPAUL_TRACE_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define XPAUL_TRACE_STR(str, str2)		fprintf(stderr, "%s, " str ": %s\n", __TBX_FUNCTION__ , (char *)(str2))
#else
#define XPAUL_TRACEF(str, ...)     (void)(0)
#define XPAUL_TRACE_IN()           (void)(0)
#define XPAUL_TRACE_OUT()          (void)(0)
#define XPAUL_TRACE_CHAR(val)      (void)(0)
#define XPAUL_TRACE_VAL(str, val)  (void)(0)
#define XPAUL_TRACE_PTR(str, ptr)  (void)(0)
#define XPAUL_TRACE_STR(str, str2) (void)(0)
#endif

#endif
