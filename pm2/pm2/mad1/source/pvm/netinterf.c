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
*/

#include <pvm3.h>

#include "sys/netinterf.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "mad_timing.h"

#ifdef PM2

#include "marcel.h"

#else

#include "safe_malloc.h"
#define lock_task()
#define unlock_task()
#define tfree    FREE
#define tmalloc  MALLOC

#endif

static char header[MAX_HEADER] __MAD_ALIGNED__;

enum { NETWORK_INIT, NETWORK_MSG };

static char cons_image[1024];

static void parse_cmd_line(int *argc, char **argv)
{
  int i, j;

  i = j = 1;
  strcpy(cons_image, "no");
  while(i<*argc) {
    if(!strcmp(argv[i], "-pm2cons")) {
      if(i == *argc-1) {
	fprintf(stderr, "Fatal error: -pm2cons option must be followed by either pm2 or xpm2.\n");
	exit(1);
      }
      strcpy(cons_image, argv[i+1]);
      i += 2;
    } else {
      argv[j++] = argv[i];
    }
    i++;
  }
  *argc = j;
  argv[j] = NULL;
}

static char *get_mad_root(void)
{
  static char buf[1024];
  char *ptr;

  if((ptr = getenv("MAD1_ROOT")) != NULL)
    return ptr;
  else {
    sprintf(buf, "%s/mad1", getenv("PM2_ROOT"));
    return buf;
  }
}

void mad_pvm_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int nhosts = 0;
  int narchs = 0;
  struct pvmhostinfo *hostlist = NULL;
  int i, j, n;
  int spmd_conf_size;
  int spmd_conf[MAX_MODULES];
  int self_tid;
  char cmd[1024], execname[1024], my_number[16], num_nodes[16];
  char *arguments[64];

   pvm_setopt(PvmRoute, PvmRouteDirect);

   self_tid = pvm_mytid();

   if(pvm_parent() == PvmNoParent) { /* master process */

     setbuf(stdout, NULL);
     dup2(STDERR_FILENO, STDOUT_FILENO);

      pvm_config(&nhosts, &narchs, &hostlist);

      spmd_conf[0] = self_tid;
      spmd_conf_size = 1;

      i = 0;
      while(hostlist[i].hi_tid != pvm_tidtohost(self_tid))
         i++;

      if(nb_proc == NETWORK_ASK_USER) {
         do {
            printf("Config. size (1-%d or %d for one proc. per node) : ", MAX_MODULES, NETWORK_ONE_PER_NODE);
            scanf("%d", &nb_proc);
         } while((nb_proc < 0 || nb_proc > MAX_MODULES) && nb_proc != NETWORK_ONE_PER_NODE);
      }

      if(nb_proc == NETWORK_ONE_PER_NODE) {
	int ret = system("exit `cat ${MAD1_ROOT-${PM2_ROOT}/mad1}/.madconf | wc -w`");

	n = WEXITSTATUS(ret) - 1;

      } else 
         n = nb_proc-1;

      if(argv[0][0] != '/') {
	getcwd(execname, 1024);
	strcat(execname, "/");
	strcat(execname, argv[0]);
	argv[0] = execname;
      }
      sprintf(cmd, "%s/bin/pvm/madstart", get_mad_root());

      sprintf(num_nodes, "%d", (strcmp(cons_image, "no") ? n+2 : n+1));
      arguments[0] = num_nodes;
      arguments[1] = my_number;
      for(j=2; j <= 3 + *argc; j++)
	arguments[j] = argv[j-2];

      parse_cmd_line(argc, argv);

      j=1;
      while(n--) {
         i++;

         if(i == nhosts)
            i = 0;
	 sprintf(my_number, "%d", j++);
         if(pvm_spawn(cmd, arguments, PvmTaskHost, hostlist[i].hi_name,
                                 1, spmd_conf+spmd_conf_size) != 1) {
            fprintf(stderr, "Error: pvm_spawn failed\n");
            exit(1);
         }
         spmd_conf_size++;
      }
      if(strcmp(cons_image, "no")) {

	sprintf(cmd, "%s/bin/pvm/madstartcons", get_mad_root());
	arguments[2] = cons_image;
	arguments[3] = NULL;

	sprintf(my_number, "%d", j++);
	if(pvm_spawn(cmd, arguments, PvmTaskHost, hostlist[0].hi_name,
		     1, spmd_conf+spmd_conf_size) != 1) {
	  fprintf(stderr, "Error: pvm_spawn failed\n");
	  exit(1);
	}
	spmd_conf_size++;
      }

      if(spmd_conf_size > 1) {
         pvm_initsend(PvmDataDefault);
         pvm_pkint(&spmd_conf_size, 1, 1);
         pvm_pkint(spmd_conf, spmd_conf_size, 1);
         for(i=1; i<spmd_conf_size; i++)
            pvm_send(spmd_conf[i], NETWORK_INIT);
      }

   } else { /* slave process */
     int myrank, f;
     char output[64];

     parse_cmd_line(argc, argv);

     pvm_recv(-1, NETWORK_INIT);
     pvm_upkint(&spmd_conf_size, 1, 1);
     pvm_upkint(spmd_conf, spmd_conf_size, 1);

     for(myrank=0; spmd_conf[myrank] != self_tid; myrank++) ;

     if(getenv("MAD_CONSOLE") == NULL) {
#ifdef PM2
       sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), myrank);
#else
       sprintf(output, "/tmp/%s-madlog-%d", getenv("USER"), myrank);
#endif
       dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
       dup2(STDOUT_FILENO, STDERR_FILENO);
       close(f);
     }
   }

   /* Everybody talk with everybody */
   for(i=0; i<spmd_conf_size; i++) {
      int j;

      if(spmd_conf[i] == self_tid) {
         pvm_initsend(PvmDataDefault);
         for(j=0; j<spmd_conf_size; j++) {
            pvm_send(spmd_conf[j], NETWORK_INIT);
            pvm_recv(spmd_conf[j], NETWORK_INIT);
         }
      } else {
         pvm_recv(spmd_conf[i], NETWORK_INIT);
         pvm_initsend(PvmDataDefault);
         pvm_send(spmd_conf[i], NETWORK_INIT);
      }
   }

   if(strcmp(cons_image, "no")) /* Hide the console to the application */
     spmd_conf_size--;

   memcpy(tids, spmd_conf, sizeof(int)*spmd_conf_size);
   *nb = spmd_conf_size;
   *whoami = self_tid;
}

void mad_pvm_network_send(int dest_node, struct iovec *vector, size_t count)
{
  int i;

  TIMING_EVENT("network_send'begin");

  lock_task();
  pvm_initsend(PvmDataRaw);
  pvm_pkint(&vector[0].iov_len, 1, 1);
  pvm_pkbyte(vector[0].iov_base, vector[0].iov_len, 1);
  for(i=1; i<count; i++) {
    pvm_pkbyte(vector[i].iov_base, vector[i].iov_len, 1);
  }

  TIMING_EVENT("pvm_send'begin");

  pvm_send(dest_node, NETWORK_MSG);

  TIMING_EVENT("pvm_send'end");

  unlock_task();
#ifdef DEBUG
  fprintf(stderr, "One message sent to %p\n", dest_node);
#endif

  TIMING_EVENT("network_send'end");

}

void mad_pvm_network_receive(char **head)
{
  size_t first_iov_len;
  int bufid;

#ifdef PM2
  while(1) {
    lock_task();
    if(marcel_self()->next == marcel_self()) {
      bufid = pvm_recv(-1, NETWORK_MSG);
      unlock_task();
      break;
    }
    bufid = pvm_nrecv(-1, NETWORK_MSG);
    unlock_task();

    if(bufid > 0)
      break;

    marcel_givehandback();
  }
#else
  bufid = pvm_recv(-1, NETWORK_MSG);
#endif

#ifdef DEBUG
  fprintf(stderr, "One message received\n");
#endif

  lock_task();
  pvm_upkint(&first_iov_len, 1, 1);
  pvm_upkbyte(header, first_iov_len, 1);
  unlock_task();

  *head = header;
}

void mad_pvm_network_receive_data(struct iovec *vector, size_t count)
{
  int i;

  lock_task();
  for(i=0; i<count; i++) {
     pvm_upkbyte(vector[i].iov_base, vector[i].iov_len, 1);
  }
  unlock_task();
}

void mad_pvm_network_exit(void)
{
  pvm_exit();
}

static netinterf_t mad_pvm_netinterf = {
  mad_pvm_network_init,
  mad_pvm_network_send,
  mad_pvm_network_receive,
  mad_pvm_network_receive_data,
  mad_pvm_network_exit
};

netinterf_t *mad_pvm_netinterf_init()
{
  return &mad_pvm_netinterf;
}

