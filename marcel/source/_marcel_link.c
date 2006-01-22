
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
 * This file is used as linker script, needs be included first while linking.
 */
#if defined(DARWIN_SYS) || defined(SOLARIS_SYS)
#include <sys/types.h>
#include "tbx_macros.h"
#include "tbx_compiler.h"

TBX_SECTION(".ma.runqueues") TBX_ALIGN(4096)	const int __ma_runqueues[0]={};
TBX_SECTION(".ma.main.lwp") TBX_ALIGN(4096)	const int __ma_main_lwp_start[0]={};
TBX_SECTION(".ma.per.lwp.main")			const int __ma_per_lwp_main[0]={};
TBX_SECTION(".ma.main.lwp.end")			const int __ma_main_lwp_end[0]={};

TBX_SECTION(".ma.debug.start") TBX_ALIGN(4096)	const int __ma_debug_start[0]={};
TBX_SECTION(".ma.debug.var")			const int __ma_debug_var[0]={};
TBX_SECTION(".ma.debug.end")			const int __ma_debug_end[0]={};

TBX_SECTION(".ma.debug.size") TBX_ALIGN(4096)	const int __ma_debug_size[0]={};
TBX_SECTION(".ma.debug.size.0")			const int __ma_debug_size_0[0]={};
TBX_SECTION(".ma.debug.size.1")			const int __ma_debug_size_1[0]={};

TBX_SECTION(".ma.init.start") TBX_ALIGN(4096)	const int __ma_init_start[0]={};
TBX_SECTION(".ma.init.ind.0")			const int __ma_init_ind_0[0]={};
TBX_SECTION(".ma.init.ind.1")			const int __ma_init_ind_1[0]={};
TBX_SECTION(".ma.init.ind.2")			const int __ma_init_ind_2[0]={};
TBX_SECTION(".ma.init.ind.3")			const int __ma_init_ind_3[0]={};
TBX_SECTION(".ma.init.ind.4")			const int __ma_init_ind_4[0]={};
TBX_SECTION(".ma.init.ind.5")			const int __ma_init_ind_5[0]={};
TBX_SECTION(".ma.init.inf.0")			const int __ma_init_inf_0[0]={};
TBX_SECTION(".ma.init.inf.0.0")			const int __ma_init_inf_0_0[0]={};
TBX_SECTION(".ma.init.inf.0.1")			const int __ma_init_inf_0_1[0]={};
TBX_SECTION(".ma.init.inf.0.2")			const int __ma_init_inf_0_2[0]={};
TBX_SECTION(".ma.init.inf.0.3")			const int __ma_init_inf_0_3[0]={};
TBX_SECTION(".ma.init.inf.0.4")			const int __ma_init_inf_0_4[0]={};
TBX_SECTION(".ma.init.inf.0.5")			const int __ma_init_inf_0_5[0]={};
TBX_SECTION(".ma.init.inf.0.6")			const int __ma_init_inf_0_6[0]={};
TBX_SECTION(".ma.init.inf.0.7")			const int __ma_init_inf_0_7[0]={};
TBX_SECTION(".ma.init.inf.0.8")			const int __ma_init_inf_0_8[0]={};
TBX_SECTION(".ma.init.inf.0.9")			const int __ma_init_inf_0_9[0]={};
TBX_SECTION(".ma.init.inf.1")			const int __ma_init_inf_1[0]={};
TBX_SECTION(".ma.init.inf.1.0")			const int __ma_init_inf_1_0[0]={};
TBX_SECTION(".ma.init.inf.1.1")			const int __ma_init_inf_1_1[0]={};
TBX_SECTION(".ma.init.inf.1.2")			const int __ma_init_inf_1_2[0]={};
TBX_SECTION(".ma.init.inf.1.3")			const int __ma_init_inf_1_3[0]={};
TBX_SECTION(".ma.init.inf.1.4")			const int __ma_init_inf_1_4[0]={};
TBX_SECTION(".ma.init.inf.1.5")			const int __ma_init_inf_1_5[0]={};
TBX_SECTION(".ma.init.inf.1.6")			const int __ma_init_inf_1_6[0]={};
TBX_SECTION(".ma.init.inf.1.7")			const int __ma_init_inf_1_7[0]={};
TBX_SECTION(".ma.init.inf.1.8")			const int __ma_init_inf_1_8[0]={};
TBX_SECTION(".ma.init.inf.1.9")			const int __ma_init_inf_1_9[0]={};
TBX_SECTION(".ma.init.inf.2")			const int __ma_init_inf_2[0]={};
TBX_SECTION(".ma.init.inf.2.0")			const int __ma_init_inf_2_0[0]={};
TBX_SECTION(".ma.init.inf.2.1")			const int __ma_init_inf_2_1[0]={};
TBX_SECTION(".ma.init.inf.2.2")			const int __ma_init_inf_2_2[0]={};
TBX_SECTION(".ma.init.inf.2.3")			const int __ma_init_inf_2_3[0]={};
TBX_SECTION(".ma.init.inf.2.4")			const int __ma_init_inf_2_4[0]={};
TBX_SECTION(".ma.init.inf.2.5")			const int __ma_init_inf_2_5[0]={};
TBX_SECTION(".ma.init.inf.2.6")			const int __ma_init_inf_2_6[0]={};
TBX_SECTION(".ma.init.inf.2.7")			const int __ma_init_inf_2_7[0]={};
TBX_SECTION(".ma.init.inf.2.8")			const int __ma_init_inf_2_8[0]={};
TBX_SECTION(".ma.init.inf.2.9")			const int __ma_init_inf_2_9[0]={};
TBX_SECTION(".ma.init.inf.3")			const int __ma_init_inf_3[0]={};
TBX_SECTION(".ma.init.inf.3.0")			const int __ma_init_inf_3_0[0]={};
TBX_SECTION(".ma.init.inf.3.1")			const int __ma_init_inf_3_1[0]={};
TBX_SECTION(".ma.init.inf.3.2")			const int __ma_init_inf_3_2[0]={};
TBX_SECTION(".ma.init.inf.3.3")			const int __ma_init_inf_3_3[0]={};
TBX_SECTION(".ma.init.inf.3.4")			const int __ma_init_inf_3_4[0]={};
TBX_SECTION(".ma.init.inf.3.5")			const int __ma_init_inf_3_5[0]={};
TBX_SECTION(".ma.init.inf.3.6")			const int __ma_init_inf_3_6[0]={};
TBX_SECTION(".ma.init.inf.3.7")			const int __ma_init_inf_3_7[0]={};
TBX_SECTION(".ma.init.inf.3.8")			const int __ma_init_inf_3_8[0]={};
TBX_SECTION(".ma.init.inf.3.9")			const int __ma_init_inf_3_9[0]={};
TBX_SECTION(".ma.init.inf.4")			const int __ma_init_inf_4[0]={};
TBX_SECTION(".ma.init.inf.4.0")			const int __ma_init_inf_4_0[0]={};
TBX_SECTION(".ma.init.inf.4.1")			const int __ma_init_inf_4_1[0]={};
TBX_SECTION(".ma.init.inf.4.2")			const int __ma_init_inf_4_2[0]={};
TBX_SECTION(".ma.init.inf.4.3")			const int __ma_init_inf_4_3[0]={};
TBX_SECTION(".ma.init.inf.4.4")			const int __ma_init_inf_4_4[0]={};
TBX_SECTION(".ma.init.inf.4.5")			const int __ma_init_inf_4_5[0]={};
TBX_SECTION(".ma.init.inf.4.6")			const int __ma_init_inf_4_6[0]={};
TBX_SECTION(".ma.init.inf.4.7")			const int __ma_init_inf_4_7[0]={};
TBX_SECTION(".ma.init.inf.4.8")			const int __ma_init_inf_4_8[0]={};
TBX_SECTION(".ma.init.inf.4.9")			const int __ma_init_inf_4_9[0]={};
TBX_SECTION(".ma.init.inf.5")			const int __ma_init_inf_5[0]={};
TBX_SECTION(".ma.init.inf.5.0")			const int __ma_init_inf_5_0[0]={};
TBX_SECTION(".ma.init.inf.5.1")			const int __ma_init_inf_5_1[0]={};
TBX_SECTION(".ma.init.inf.5.2")			const int __ma_init_inf_5_2[0]={};
TBX_SECTION(".ma.init.inf.5.3")			const int __ma_init_inf_5_3[0]={};
TBX_SECTION(".ma.init.inf.5.4")			const int __ma_init_inf_5_4[0]={};
TBX_SECTION(".ma.init.inf.5.5")			const int __ma_init_inf_5_5[0]={};
TBX_SECTION(".ma.init.inf.5.6")			const int __ma_init_inf_5_6[0]={};
TBX_SECTION(".ma.init.inf.5.7")			const int __ma_init_inf_5_7[0]={};
TBX_SECTION(".ma.init.inf.5.8")			const int __ma_init_inf_5_8[0]={};
TBX_SECTION(".ma.init.inf.5.9")			const int __ma_init_inf_5_9[0]={};
TBX_SECTION(".ma.init.end")			const int __ma_init_end[0]={};

#endif
