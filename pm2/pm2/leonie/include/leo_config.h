
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
