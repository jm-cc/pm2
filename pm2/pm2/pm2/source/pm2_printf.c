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
$Log: pm2_printf.c,v $
Revision 1.6  2000/07/14 16:17:13  gantoniu
Merged with branch dsm3

Revision 1.5.2.1  2000/06/13 16:44:12  gantoniu
New dsm branch.

Revision 1.4  2000/02/28 11:17:14  rnamyst
Changed #include <> into #include "".

Revision 1.3  2000/01/31 15:58:29  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include "pm2.h"
#include "sys/pm2_printf.h"

#include <stdarg.h>

static unsigned PM2_PRINTF;

static marcel_sem_t print_mutex;
static char _pm2_print_buf[1024] __MAD_ALIGNED__;

extern int pm2_main_module(void);

void pm2_printf(char *format, ...)
{ 
  va_list args;
  char *ptr;

  if(pm2_main_module() == __pm2_self) {
    lock_task();
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    unlock_task();
  } else {
    marcel_sem_P(&print_mutex);
    sprintf(_pm2_print_buf, "[t%x] ", __pm2_self);
    ptr = _pm2_print_buf + strlen(_pm2_print_buf);
    va_start(args, format);
    vsprintf(ptr, format, args);
    va_end(args);
    {
      unsigned len = strlen(_pm2_print_buf)+1;

      pm2_rawrpc_begin(pm2_main_module(), PM2_PRINTF, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, _pm2_print_buf, len);
      pm2_rawrpc_end();
    }
    marcel_sem_V(&print_mutex);
  }
}

static void netserver_printf(void)
{ 
  static char buf[2048] __MAD_ALIGNED__;
  unsigned len;

  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, buf, len);

  pm2_rawrpc_waitdata();

  tfprintf(stderr, "%s", buf);
}

void pm2_printf_init(void)
{
  marcel_sem_init(&print_mutex, 1);

  pm2_rawrpc_register(&PM2_PRINTF, netserver_printf);
}

