/*
 * Copyright (c) 2004, Intel Corporation. All rights reserved.
 * This file is licensed under the GPL license.  For the full content 
 * of this license, see the COPYING file at the top level of this 
 * source tree.
 *
 * adam.li@intel.com
 * If the Timers option is supported, the timeout shall be based on 
 * the CLOCK_REALTIME clock. If the Timers option is not supported, 
 * the timeout shall be based on the system clock as returned by 
 * the time() function.
 * 
 */


#define WITH_TIME
#include "sem2.c"


