
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


#include "marcel.h"
#include "tbx_compiler.h"

#ifdef PM2DEBUG
MA_DEBUG_DEFINE_NAME_DEPEND(default, &marcel_mdebug);

MA_DEBUG_DEFINE_STANDARD(marcel_default, "marcel-default");

MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_debug=
  NEW_DEBUG_TYPE_DEPEND("MA: ", "ma", NULL);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_mdebug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-mdebug", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_debug_state=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-state", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_debug_work=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-work", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_debug_deviate=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-deviate", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_mdebug_sched_q=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-mdebug-sched-q", &marcel_debug);

MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_heap_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-heap-debug", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_mami_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-mami-debug", &marcel_debug);

#ifdef DEBUG_SCHED
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_sched_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-sched", &marcel_debug);
#endif

#ifdef DEBUG_BUBBLE_SCHED
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_bubble_sched_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-bubble-sched", &marcel_debug);
#endif

#ifdef MARCEL_TRACE
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_mtrace=
  NEW_DEBUG_TYPE_DEPEND("MAR_TRACE: ", "mar-trace", &marcel_debug);
MA_DEBUG_VAR_ATTRIBUTE debug_type_t marcel_mtrace_timer=
  NEW_DEBUG_TYPE_DEPEND("MAR_TRACE: ", "mar-trace-timer", &marcel_debug);
#endif

#endif //PM2DEBUG

void marcel_debug_init(int* argc TBX_UNUSED, char** argv TBX_UNUSED, int debug_flags TBX_UNUSED)
{
	static int called=0;
	if (called) 
		return;
	called=1;

	pm2debug_init_ext(argc, argv, debug_flags);
}

typedef struct { debug_type_t d;} TBX_ALIGNED debug_type_aligned_t;

#ifdef PM2DEBUG
static debug_type_t ma_dummy1 TBX_SECTION(".ma.debug.size.0") TBX_ALIGNED =  NEW_DEBUG_TYPE("dummy", "dummy");
static debug_type_t ma_dummy2 TBX_SECTION(".ma.debug.size.1") TBX_ALIGNED =  NEW_DEBUG_TYPE("dummy", "dummy");
extern debug_type_aligned_t __ma_debug_pre_start[];
extern debug_type_aligned_t __ma_debug_start[];
extern debug_type_aligned_t __ma_debug_end[];
#endif

static void __marcel_init marcel_debug_init_auto(void)
{
#ifdef PM2DEBUG
	debug_type_aligned_t *var;
	unsigned long __ma_debug_size_entry;
	unsigned long TBX_UNUSED __ma_debug_size=(void*)&(__ma_debug_start[1])-(void*)__ma_debug_start;
	if (&ma_dummy2 < &ma_dummy1)
		__ma_debug_size_entry = (void*)&ma_dummy1-(void*)&ma_dummy2;
	else
		__ma_debug_size_entry = (void*)&ma_dummy2-(void*)&ma_dummy1;

	MA_BUG_ON(__ma_debug_size_entry != __ma_debug_size);
	for(var=__ma_debug_start; var < __ma_debug_end; var++) {
		pm2debug_register(&var->d);
	}
#endif
}

__ma_initfunc_prio(marcel_debug_init_auto, MA_INIT_DEBUG, MA_INIT_DEBUG_PRIO, "Register debug variables");
