
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

#section marcel_macros

#define MA_DECLARE_DEBUG_NAME(name) MA_DECLARE_DEBUG_NAME_S(name)
#define MA_DECLARE_DEBUG_NAME_S(name) extern debug_type_t ma_debug_##name

#define MA_DEFINE_DEBUG_NAME(name, show_init) \
  MA_DEFINE_DEBUG_NAME_S(name, show_init)
#define MA_DEFINE_DEBUG_NAME_S(name, show_init) \
  debug_type_t ma_debug_##name __attribute__((section(".ma.debug.var"))) \
  =  NEW_DEBUG_TYPE(show_init, "ma-"#name": ", "ma-"#name)

#define ma_debug(name, fmt, args...) ma_debug_s(name, fmt , ##args)
#define ma_debug_s(name, fmt, args...) \
  debug_printf(&ma_debug_##name, fmt , ##args)


#ifdef MA_FILE_DEBUG
/* Une variable de debug à définir pour ce fichier C */
#ifndef MA_FILE_DEBUG_NO_DEFINE
/* On peut avoir envie de le définir nous même (à 1 par exemple) */
MA_DEFINE_DEBUG_NAME(MA_FILE_DEBUG, 0);
#endif
#define debug(fmt, args...) \
  ma_debug(MA_FILE_DEBUG, fmt , ##args)
#endif

