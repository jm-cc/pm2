/*
 * NewMadeleine
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


#ifdef CONFIG_LOG
#define NM_SO_SR_LOG_IN()	    fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define NM_SO_SR_LOG_OUT()	    fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#else
#define NM_SO_SR_LOG_IN()           (void)(0)
#define NM_SO_SR_LOG_OUT()          (void)(0)
#endif
