
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
