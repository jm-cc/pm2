
/*
 * CVS Id: $Id: hierarch_topology.c,v 1.9 2002/11/29 18:44:20 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA / INRIA, May 2002 */

/* This module stores the information about the underlying topology of
 * the nodes connected over a hierarchical network.  It offers 'set'
 * functions for the user to specify the topology, and 'get' functions
 * for the token_locks and the hierarchical consistency protocol. */


#include "dsm_const.h"   /* return codes */
#include "marcel.h"   /* tmalloc() */
#include "pm2.h"   /* pm2_config_size() */


/**********************************************************************/
/* PRIVATE GLOBAL VARIABLES                                           */
/**********************************************************************/

static int cluster_number = 0;
static int * cluster_colors = NULL;


/**********************************************************************/
/* PUBLIC GLOBAL VARIABLES                                            */
/**********************************************************************/

const int NO_COLOR = ((int)(-1));


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

/**********************************************************************/
/* initialization of the topology information to a flat hierarchy by
 * default; this function is called by dsm_pm2_init(); only one thread
 * should execute this function. */
int
topology_initialization (void)
{
   int i;
   const dsm_node_t size = pm2_config_size();

   cluster_number = 1;

   cluster_colors = tmalloc(size * sizeof(int));
   if ( cluster_colors == NULL ) return DSM_ERR_MEMORY;
   for (i = 0; i < size; i++)
      cluster_colors[i] = 0;

   return DSM_SUCCESS;
}


/**********************************************************************/
/* finalization function for the topology information; this function
 * is called by dsm_pm2_exit(); only one thread should execute this
 * function. */
int
topology_finalization (void)
{
   tfree(cluster_colors);
   cluster_colors = NULL;
   cluster_number = 0;

   return DSM_SUCCESS;
}


/**********************************************************************/
/* return the number of clusters; this function is NOT protected
 * against concurrency and must be called after pm2_init(). */
int
topology_get_cluster_number (void)
{
   return cluster_number;
}


/**********************************************************************/
/* return the color of the given node to determine to what cluster it
 * belongs; this function is NOT protected against concurrency and
 * must be called after pm2_init(). */
int
topology_get_cluster_color (const dsm_node_t node)
{
   if ( node < pm2_config_size() && node != NOBODY )
      return cluster_colors[node];
   return NO_COLOR;
}


/**********************************************************************/
/* set the colors of the processes, depending on what cluster they
 * belong to: 2 processes with the same color belong to the same
 * cluster; this function is NOT protected against concurrency and
 * must be called after pm2_init(). */
int
topology_set_cluster_colors (const int * const tab)
{
   int i;
   const int size = pm2_config_size();

   cluster_number = 0;
   for (i = 0; i < size; i++)
   {
      cluster_colors[i] = tab[i];
      if ( tab[i] > cluster_number )
         cluster_number = tab[i];
   }
   cluster_number++;

   return DSM_SUCCESS;
}


/**********************************************************************/
/* this function dumps on stderr all the topology information (for
 * debugging purpose...); this function is NOT protected against
 * concurrency and must be called after pm2_init(). */
int
topology_dump_information (void)
{
   int i;
   const int size = pm2_config_size();

   marcel_fprintf(stderr, "Maximum number of clusters (colors): %d\n",
            cluster_number);
   marcel_fprintf(stderr, "Node:\t");
   for (i = 0; i < size; i++)
      marcel_fprintf(stderr, "%3d", i);
   marcel_fprintf(stderr, "\nColor:\t");
   for (i = 0; i < size; i++)
      marcel_fprintf(stderr, "%3u", cluster_colors[i]);
   marcel_fprintf(stderr, "\n");

   return DSM_SUCCESS;
}

