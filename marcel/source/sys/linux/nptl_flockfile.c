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


#include "sys/linux/nptl_flockfile.h"


#ifdef MA__LIBPTHREAD


void __flockfile(_IO_FILE *stream)
{
	lpt_io_file *__stream;

	if (tbx_likely(marcel_test_activity())) {
		__stream = (lpt_io_file *)stream;
		__lpt_lock(&__stream->_lock, ma_self());
	}
}
DEF_STRONG_ALIAS(__flockfile, _IO_flockfile)
DEF_WEAK_ALIAS(__flockfile, flockfile)

void __funlockfile(_IO_FILE *stream)
{
	lpt_io_file *__stream;

	if (tbx_likely(marcel_test_activity())) {
		__stream = (lpt_io_file *)stream;
		__lpt_unlock(&__stream->_lock);
	}
}
DEF_STRONG_ALIAS(__funlockfile, _IO_funlockfile)
DEF_WEAK_ALIAS(__funlockfile, funlockfile)

int __ftrylockfile(_IO_FILE *stream)
{
	lpt_io_file *__stream;

	if (tbx_likely(marcel_test_activity())) {
		__stream = (lpt_io_file *)stream;
		return __lpt_trylock(&__stream->_lock);
	}

	return 0;
}
DEF_STRONG_ALIAS(__ftrylockfile, _IO_ftrylockfile)
DEF_WEAK_ALIAS(__ftrylockfile, ftrylockfile)


#endif /** MA__LIBPTHREAD **/
