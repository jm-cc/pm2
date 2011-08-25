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


#include "tbx_timing.h"
#include "tbx_compiler.h"
#include "fut_pm2.h"


void __cyg_profile_func_enter (void *this_fn, void *call_site) TBX_VISIBILITY("default");
TBX_VISIBILITY("default") void __cyg_profile_func_enter (void *this_fn, void *call_site TBX_UNUSED)
{
#ifdef IA64_ARCH
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_ENTRY_CODE,
		   *((void**)this_fn));
#else
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_ENTRY_CODE,
		   this_fn);
#endif
}

void __cyg_profile_func_exit  (void *this_fn, void *call_site) TBX_VISIBILITY("default");
TBX_VISIBILITY("default") void __cyg_profile_func_exit  (void *this_fn, void *call_site TBX_UNUSED)
{
#ifdef IA64_ARCH
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_EXIT_CODE,
		   *((void**)this_fn));
#else
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_EXIT_CODE,
		   this_fn);
#endif
}
