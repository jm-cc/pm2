
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * Leo_swann.h
 * ===========
 */ 

#ifndef __LEO_SWANN_H
#define __LEO_SWANN_H

p_leo_swann_module_t
leo_launch_swann_module(p_leonie_t                  leonie,
			p_leo_application_cluster_t cluster);

p_leo_mad_module_t
leo_launch_mad_module(p_leonie_t                  leonie,
		      p_leo_application_cluster_t cluster);

#endif /* __LEO_SWANN_H */
