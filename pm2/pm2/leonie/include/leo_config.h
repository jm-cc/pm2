
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
 * Leo_config.h
 * ============
 */ 
#ifndef __LEO_CONFIG_H
#define __LEO_CONFIG_H

char *
leo_find_string(p_tbx_list_t  list,
		char         *string);

p_leo_clu_host_name_t
leo_get_cluster_entry_point(p_leo_clu_cluster_t  cluster,
			    char                *cluster_id);

void
leo_cluster_setup(p_leonie_t               leonie,
		  p_leo_app_application_t  application,
		  p_leo_clu_cluster_file_t local_cluster_def);

#endif /* __LEO_CONFIG_H */
