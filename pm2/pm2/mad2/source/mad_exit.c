/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: mad_exit.c,v $
Revision 1.1  2000/05/17 14:34:14  oaumage
- Reorganisation des sources au niveau de mad_init


______________________________________________________________________________
*/

/*
 * Mad_exit.c
 * ==========
 */
/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

static void
mad_foreach_adapter_exit(void *object)
{
  p_mad_adapter_t          adapter   = object;
  p_mad_driver_t           driver    = adapter->driver;
  p_mad_driver_interface_t interface = &(driver->interface);
  
  LOG_IN();
  if (interface->adapter_exit)
    {
      interface->adapter_exit(adapter);
    }
  else
    {
      if (adapter->specific)
	{
	  TBX_FREE(adapter->specific);
	}
    }
  LOG_OUT();
}

static void
mad_driver_exit(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t           driver;
      p_mad_driver_interface_t interface;
      
      driver    = &(madeleine->driver[drv]);      
      interface = &(driver->interface);

      tbx_foreach_destroy_list(&(madeleine->channel),
			       mad_foreach_adapter_exit);
      if (interface->driver_exit)
	{
	  interface->driver_exit(driver);
	}
      else
	{
	  if (driver->specific)
	    {
	      TBX_FREE(driver->specific);
	    }
	}
    }

  TBX_FREE(madeleine->adapter);
  TBX_FREE(madeleine->driver);
  LOG_OUT();
}

#ifdef PM2
void
mad2_exit(p_mad_madeleine_t madeleine)
#else
void
mad_exit(p_mad_madeleine_t madeleine)
#endif
{
  LOG_IN();
  TBX_LOCK_SHARED(madeleine);
  mad_close_channels(madeleine);
  mad_driver_exit(madeleine);
  tbx_list_manager_exit();
  TBX_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}


