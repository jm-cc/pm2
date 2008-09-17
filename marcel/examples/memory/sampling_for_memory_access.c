/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#if !defined(MARCEL_MAMI_ENABLED)
#error This application needs MAMI to be enabled
#endif

int marcel_main(int argc, char **argv) {
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_memory_init(&memory_manager, 1000);

  marcel_memory_sampling_of_memory_access(&memory_manager);

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
}
