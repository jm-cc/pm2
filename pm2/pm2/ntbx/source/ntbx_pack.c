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
$Log: ntbx_pack.c,v $
Revision 1.3  2000/04/27 08:59:19  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.2.2.4  2000/04/15 15:23:29  oaumage
- corrections diverses

Revision 1.2.2.3  2000/04/15 15:20:36  oaumage
- corrections diverses

Revision 1.2.2.2  2000/04/13 15:54:29  oaumage
- correction des fonctions pack/unpack

Revision 1.2.2.1  2000/04/03 13:46:23  oaumage
- ajout de fonctions d'empaquetage (donnees long)

Revision 1.2  2000/03/01 11:03:35  oaumage
- mise a jour des #includes ("")

Revision 1.1  2000/02/17 09:09:07  oaumage
- ajout du support TCP a la net toolbox
- ajout des fonctions d'empaquetage de donnees numeriques


______________________________________________________________________________
*/

/*
 * ntbx_pack.c
 * -----------
 */
/* #define DEBUG */
#include "ntbx.h"
#include <stdlib.h>
#include <string.h>

/*
 * Integer
 * -------
 */
void
ntbx_pack_int(int                  data,
	      p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "I");
    }
  sprintf(ptr,
	  "%*d%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_VAL("data", data);
  LOG_STR("pack_buffer", pack_buffer);
  LOG_OUT();
}

int
ntbx_unpack_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer;
  int   data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'I')
	FAILURE("synchronisation error");
    }
  data = atoi(ptr);
  LOG_VAL("data", data);
  LOG_STR("pack_buffer", pack_buffer);
  LOG_OUT();
  return data;
}

/*
 * Long integer
 * ------------
 */
void
ntbx_pack_long(long                 data,
	       p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "L");
    }
  sprintf(ptr,
	  "%*ld%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_OUT();
}

long
ntbx_unpack_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer;
  long  data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'L')
	FAILURE("synchronisation error");
    }
  data = atol(ptr);
  LOG_OUT();
  return data;
}

/*
 * Double
 * ------
 */
void
ntbx_pack_double(double               data,
		 p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "D");
    }
  sprintf(ptr,
	  "%*f%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_OUT();
}

int
ntbx_unpack_double(p_ntbx_pack_buffer_t pack_buffer)
{
  void   *ptr = pack_buffer;
  double  data;
  int     i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'D')
	FAILURE("synchronisation error");
    }
  data = atof(ptr);
  LOG_OUT();
  return data;
}
