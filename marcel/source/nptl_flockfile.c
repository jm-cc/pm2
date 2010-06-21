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


#include "tbx_types.h"
#include "nptl_flockfile.h"
#include "marcel_alias.h"


/* duplicate from marcel_init.h --> minimize file inclusion
   and collision between real f*lockfile declaration and marcel ones */
extern tbx_flag_t ma_activity;


#ifdef MA__LIBPTHREAD
#ifdef PM2_DEV
#  warning workaround: marcel is started too late
#endif


void __flockfile(_LPT_IO_FILE *stream)
{
	if (ma_activity)
		_lpt_IO_lock_lock(&(*stream->_lock));
}
DEF_STRONG_ALIAS(__flockfile, _IO_flockfile)
DEF_WEAK_ALIAS(__flockfile, flockfile)

void __funlockfile(_LPT_IO_FILE *stream)
{
	if (ma_activity)
		_lpt_IO_lock_unlock(&(*stream->_lock));
}
DEF_STRONG_ALIAS(__funlockfile, _IO_funlockfile)
DEF_WEAK_ALIAS(__funlockfile, funlockfile)

int __ftrylockfile(_LPT_IO_FILE *stream)
{
	if (ma_activity)
		return _lpt_IO_lock_trylock(&(*stream->_lock));
	return 0;
}
DEF_STRONG_ALIAS(__ftrylockfile, _IO_ftrylockfile)
DEF_WEAK_ALIAS(__ftrylockfile, ftrylockfile)


#endif /** MA__LIBPTHREAD **/
