
/*
 * CVS Id: $Id: hierarch_topology.h,v 1.4 2002/10/19 15:13:30 slacour Exp $
 */

#include "dsm_const.h"   /* dsm_node_t */


const unsigned int NO_COLOR;


/* initialization function of the topology, called from
 * dsm_pm2_init(); only one thread should execute this function. */
extern int
topology_initialization (void);

/* finalization function called from dsm_pm2_exit(); only one thread
 * should execute this function. */
extern int
topology_finalization (void);

/* get the number of clusters; this function is not protected against
 * concurrency. */
extern unsigned int
topology_get_cluster_number (void);

/* get the color of a node to determine to what cluster it belongs;
 * this function is not protected against concurrency. */
extern unsigned int
topology_get_cluster_color (const dsm_node_t);

/* set the colors of the processes, depending on what cluster they
 * belong to: 2 processes with the same color belong to the same
 * cluster; this function is NOT protected against concurrency. */
extern int
topology_set_cluster_colors (const unsigned int * const);

