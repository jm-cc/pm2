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
$Log: madeleine.c,v $
Revision 1.34  2000/09/12 09:39:40  oaumage
- correction des problemes de log

Revision 1.33  2000/07/13 10:04:20  oaumage
- Correction de l'identificateur d'adaptateur

Revision 1.32  2000/06/16 14:03:51  oaumage
- Mise a jour par rapport au nouveau fonctionnement de pm2conf

Revision 1.31  2000/05/25 00:23:39  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.30  2000/05/17 14:34:16  oaumage
- Reorganisation des sources au niveau de mad_init

Revision 1.29  2000/05/16 14:17:26  oaumage
- correction de marcel_init dans mad_pre_init

Revision 1.28  2000/05/16 09:50:44  oaumage
- suppression du DEBUG

Revision 1.27  2000/05/15 08:08:08  oaumage
- corrections diverses

Revision 1.26  2000/04/20 13:32:07  oaumage
- modifications diverses

Revision 1.25  2000/04/05 16:08:14  oaumage
- retablissement du rsh supplementaire

Revision 1.24  2000/03/27 08:50:55  oaumage
- pre-support decoupage de groupes
- correction au niveau du support du demarrage manuel

Revision 1.23  2000/03/15 16:59:16  oaumage
- support APPLICATION_SPAWN

Revision 1.22  2000/03/08 17:19:17  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

Revision 1.21  2000/03/02 15:45:48  oaumage
- support du polling Nexus

Revision 1.20  2000/03/02 14:50:54  oaumage
- support de detection des protocoles au niveau runtime

Revision 1.19  2000/03/02 09:52:17  jfmehaut
pilote Madeleine II/BIP

Revision 1.18  2000/03/01 16:44:54  oaumage
- get_mad_root --> mad_get_mad_root
- correction du passage du chemin de l'application

Revision 1.17  2000/03/01 14:15:50  oaumage
- correction sur get_mad_root

Revision 1.16  2000/03/01 14:09:07  oaumage
- prise en compte d'un fichier de configuration par defaut

Revision 1.15  2000/03/01 11:00:53  oaumage
- suppression de mad_get_root en compilation standalone

Revision 1.14  2000/02/28 11:06:17  rnamyst
Changed #include "" into #include <>.

Revision 1.13  2000/02/10 11:18:01  rnamyst
Modified to use default values for environment variables...

Revision 1.12  2000/02/08 17:49:07  oaumage
- support de la net toolbox

Revision 1.11  2000/02/01 17:26:33  rnamyst
PM2 compatibility functions have moved to pm2_mad.

Revision 1.10  2000/01/31 15:53:37  oaumage
- mad_channel.c : verrouillage au niveau des canaux au lieu des drivers
- madeleine.c : deplacement de aligned_malloc vers la toolbox

Revision 1.9  2000/01/21 17:27:28  oaumage
- mise a jour de l'interface de compatibilite /mad1 et renommage des anciennes
  fonctions

Revision 1.8  2000/01/13 14:45:58  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.7  2000/01/05 09:42:58  oaumage
- mad_communication.c: support du nouveau `group_mode' (a surveiller !)
- madeleine.c: external_spawn_init n'est plus indispensable
- mad_list_management: rien de particulier, mais l'ajout d'une fonction
  d'acces a la longueur de la liste est a l'etude, pour permettre aux drivers
  de faire une optimisation simple au niveau des groupes de buffers de
  taille 1

Revision 1.6  2000/01/04 16:49:10  oaumage
- madeleine: corrections au niveau du support `external spawn'
- support des fonctions non definies dans les drivers

Revision 1.5  1999/12/15 17:31:28  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Madeleine.c
 * ===========
 */

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <netdb.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

/*
 * Driver registration
 * -------------------
 */
static void (*mad_driver_registration[])(p_mad_driver_t driver) =
{
#ifdef DRV_TCP
  mad_tcp_register,
#endif /* DRV_TCP */
#ifdef DRV_VIA
  mad_via_register,
#endif /* DRV_VIA */
#ifdef DRV_SISCI
  mad_sisci_register,
#endif /* DRV_SISCI */
#ifdef DRV_SBP
  mad_sbp_register,
#endif /* DRV_SBP */
#ifdef DRV_MPI
  mad_mpi_register,
#endif /* DRV_MPI */
#ifdef DRV_BIP
  mad_bip_register,
#endif /* DRV_BIP */
  NULL
};

int
mad_protocol_available(p_mad_madeleine_t madeleine, char *name)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < mad_driver_number;
       drv++)
    {
      p_mad_driver_t driver = &madeleine->driver[drv];
      
      if (!strcmp(name, driver->name))
	{
	  LOG_OUT();
	  return drv;
	}
    }

  LOG_OUT();
  return -1;
}

/* ---- */

p_mad_adapter_set_t 
mad_adapter_set_init(int nb_adapter, ...)
{
  p_mad_adapter_set_t  set;
  int                  i;
  va_list              pa;

  LOG_IN();
  va_start(pa, nb_adapter);
  
  LOG_VAL("nb_adapter", nb_adapter);
  
  set = TBX_MALLOC(sizeof(mad_adapter_set_t));
  CTRL_ALLOC(set);
  set->size = nb_adapter;
  set->description =
    TBX_MALLOC(nb_adapter * sizeof(mad_adapter_description_t));
  CTRL_ALLOC(set->description);
  for (i = 0;
       i < nb_adapter;
       i++)
    {
      p_mad_adapter_description_t description ;

      description                   = &(set->description[i]);
      description->driver_id        = va_arg(pa, mad_driver_id_t);
      description->adapter_selector = va_arg(pa, char *);
    }

  LOG_OUT();
  va_end(pa);
  return set;
}

void 
mad_managers_init(int   *argc,
		  char **argv)
{
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT);
#ifdef MARCEL  
  marcel_init_ext(argc, argv, PM2DEBUG_DO_OPT);
#endif /* MARCEL */
  tbx_init(argc, argv, PM2DEBUG_DO_OPT);

  LOG_IN();
  ntbx_init();
  mad_memory_manager_init();
  LOG_OUT();
}

void 
mad_driver_fill(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  madeleine->driver = TBX_MALLOC(mad_driver_number * sizeof(mad_driver_t));
  CTRL_ALLOC(madeleine->driver);
  madeleine->nb_driver = mad_driver_number;
  
  for (drv = 0;
       drv < mad_driver_number;
       drv++)
    {
      p_mad_driver_t driver;
      
      driver = &(madeleine->driver[drv]);
      
      TBX_INIT_SHARED(driver);
      driver->madeleine = madeleine;
      driver->id        = drv;
      driver->specific  = NULL;
      driver->name      = NULL;

      tbx_list_init(&(driver->adapter_list));
      mad_driver_registration[drv](driver);
    }
  
  LOG_OUT();
}

void 
mad_adapter_fill(p_mad_madeleine_t     madeleine,
		 p_mad_adapter_set_t   adapter_set)
{
  mad_adapter_id_t ad;

  LOG_IN();

  if (!adapter_set)
    {      
      madeleine->adapter = TBX_MALLOC(mad_driver_number * sizeof(mad_adapter_t));
      CTRL_ALLOC(madeleine->adapter);

      madeleine->nb_adapter = mad_driver_number;

      /* Local Init */
      for (ad = 0;
	   ad < madeleine->nb_adapter;
	   ad++)
	{
	  p_mad_driver_t                driver;
	  p_mad_driver_interface_t      interface;
	  p_mad_adapter_t               adapter;
      
	  driver      = &(madeleine->driver[ad]);
	  interface   = &(driver->interface);
	  adapter     = &(madeleine->adapter[ad]);

	  TBX_INIT_SHARED(adapter);
	  tbx_append_list(&(driver->adapter_list), adapter);
      
	  adapter->driver                  = driver;
	  adapter->id                      = ad;
	  adapter->name                    = NULL;
	  adapter->master_parameter        = NULL;
	  adapter->parameter               = NULL;
	  adapter->specific                = NULL;
	}
    }
  else
    {
      madeleine->adapter = TBX_MALLOC(adapter_set->size * sizeof(mad_adapter_t));
      CTRL_ALLOC(madeleine->adapter);

      madeleine->nb_adapter = adapter_set->size;
  
      /* Local Init */
      for (ad = 0;
	   ad < madeleine->nb_adapter;
	   ad++)
	{
	  p_mad_adapter_description_t   description;
	  p_mad_driver_t                driver;
	  p_mad_driver_interface_t      interface;
	  p_mad_adapter_t               adapter;
      
	  description = &(adapter_set->description[ad]);
	  driver      = &(madeleine->driver[description->driver_id]);
	  interface   = &(driver->interface);
	  adapter     = &(madeleine->adapter[ad]);

	  TBX_INIT_SHARED(adapter);
	  tbx_append_list(&(driver->adapter_list), adapter);
      
	  adapter->driver                  = driver;
	  adapter->id                      = ad;
	  adapter->name                    = description->adapter_selector;
	  adapter->master_parameter        = NULL;
	  adapter->parameter               = NULL;
	  adapter->specific                = NULL;
	}
    }
  LOG_OUT();
}

void
mad_adapter_init(p_mad_madeleine_t madeleine)
{
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      adapter->specific = NULL;
      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
    }
  LOG_OUT();
}

void
mad_adapter_configuration_init(p_mad_madeleine_t madeleine)
{
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      if (adapter->driver->interface.adapter_configuration_init)
	adapter->driver->interface.adapter_configuration_init(adapter);
    }
  LOG_OUT();
}

char *
mad_get_mad_root(void)
{
  static char buf[1024];
  char *ptr;

  if((ptr = getenv("MAD2_ROOT")) != NULL)
    return ptr;
  else {
    sprintf(buf, "%s/mad2", getenv("PM2_ROOT"));
    return buf;
  }
}

