
/*
 * CVS Id: $Id: hierarch_topology.h,v 1.2 2002/10/14 15:42:05 slacour Exp $
 */

#include "dsm_const.h"   /* dsm_node_t */


/* initialization function of the topology, called from dsm_pm2_init() */
extern int
topology_initialization (void);

/* finalization function called from dsm_pm2_exit() */
extern int
topology_finalization (void);

/* get the number of clusters */
extern unsigned int
topology_get_cluster_number (void);

/* get the color of a node to determine to what cluster it belongs */
extern unsigned int
topology_get_cluster_color (const dsm_node_t);

