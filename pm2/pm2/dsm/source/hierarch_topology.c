
/*
 * CVS Id: $Id: hierarch_topology.c,v 1.3 2002/10/16 15:53:45 slacour Exp $
 */


#include "dsm_const.h"   /* return codes */
#include "marcel.h"   /* tmalloc() */
#include "pm2.h"   /* pm2_config_size() */


/**********************************************************************/
/* PRIVATE GLOBAL VARIABLES                                           */
/**********************************************************************/

static unsigned int cluster_number = 0;
static unsigned int * cluster_colors = NULL;


/**********************************************************************/
/* PUBLIC GLOBAL VARIABLES                      */
/**********************************************************************/

const unsigned int NO_COLOR = ((unsigned int)(-1));


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

/**********************************************************************/
/* initialization of the topology information to a flat hierarchy by
 * default; this function is called by dsm_pm2_init() */
int
topology_initialization (void)
{
   int i;
   const dsm_node_t size = pm2_config_size();

   cluster_number = 1;

   cluster_colors = tmalloc(size * sizeof(unsigned int));
   if ( cluster_colors == NULL ) return DSM_ERR_MEMORY;
   for (i = 0; i < size; i++)
      cluster_colors[i] = 0;

   return DSM_SUCCESS;
}


/**********************************************************************/
/* finalization function for the topology information; this function
 * is called by dsm_pm2_exit() */
int
topology_finalization (void)
{
   tfree(cluster_colors);
   cluster_colors = NULL;
   cluster_number = 0;

   return DSM_SUCCESS;
}


/**********************************************************************/
/* return the number of clusters */
unsigned int
topology_get_cluster_number (void)
{
   return cluster_number;
}


/**********************************************************************/
/* return the color of the given node to determine to what cluster it
 * belongs */
unsigned int
topology_get_cluster_color (const dsm_node_t node)
{
   if ( node < pm2_config_size() )
      return cluster_colors[node];
   return NO_COLOR;
}


/**********************************************************************/
/* set the colors of the processes, depending on what cluster they
 * belong to: 2 processes with the same color belong to the same
 * cluster */
int
topology_set_cluster_colors (const unsigned int * const tab)
{
   int i;
   const int size = pm2_config_size();

   for (i = 0; i < size; i++)
      cluster_colors[i] = tab[i];

   return DSM_SUCCESS;
}

