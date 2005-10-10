
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

#section macros
#define MA_HAVE_COMPAREEXCHANGE 1
#define MA_HAVE_FULLCOMPAREEXCHANGE 1
#section marcel_macros
#depend "asm/linux_intrinsics.h[marcel_functions]"
#depend "asm/linux_intrinsics.h[marcel_macros]"
#define pm2_compareexchange(p,o,n,s) ma_ia64_cmpxchg(acq,(p),(o),(n),(s))
#section marcel_inline
