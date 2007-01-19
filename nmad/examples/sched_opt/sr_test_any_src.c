/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>

#include <nm_so_sendrecv_interface.h>

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#elif defined CONFIG_GM
#  include <nm_gm_public.h>
#elif defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#elif defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#else
#  include <nm_tcp_public.h>
#endif

#define LOOPS    1
#define MAX      (64 * 1024)

const char *msg_beg	= "hello...", *msg_end = "...world!";

static void store_big_string(char *big_buf, unsigned max)
{
  const char *src;
  char *dst;

  memset(big_buf, ' ', max);
  dst = big_buf;
  src = msg_beg;
  while(*src)
    *dst++ = *src++;

  dst = big_buf + max - strlen(msg_end);
  src = msg_end;
  while(*src)
    *dst++ = *src++;
}

static
void
usage(void) {
        fprintf(stderr, "usage: hello [[-h <remote_hostname>] <remote url>]\n");
        exit(EXIT_FAILURE);
}

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        char			*r_url		= NULL;
        char			*l_url		= NULL;
        uint8_t			 drv_id		=    0;
        uint8_t			 gate_id	=    0;
        char			*hostname	= "localhost";
	struct nm_so_interface  *interface;  
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

	err = nm_so_sr_init(p_core, &interface);
	if(err != NM_ESUCCESS) {
	  printf("nm_so_sr_init return err = %d\n", err);
	  goto out;
	}

        argc--;
        argv++;

        if (argc) {
                /* client */
                while (argc) {
                        if (!strcmp(*argv, "-h")) {
                                argc--;
                                argv++;

                                if (!argc)
                                        usage();

                                hostname = *argv;
                        } else {
                                r_url	= *argv;
                        }

                        argc--;
                        argv++;
                }

                printf("running as client using remote url: %s[%s]\n", hostname, r_url);
        } else {
                /* no args: server */
                printf("running as server\n");
        }

#if defined CONFIG_MX
        err = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
        err = nm_core_driver_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_QSNET
        err = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id, &l_url);
#else
        err = nm_core_driver_init(p_core, nm_tcp_load, &drv_id, &l_url);
#endif
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        if (!r_url) {
	  int k;
                /* server
                 */
                printf("local url: [%s]\n", l_url);

                err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept returned err = %d\n", err);
                        goto out;
                }

		for(k = 0; k < LOOPS; k++) {
		  nm_so_request_t request=(nm_so_request_t) malloc(sizeof(nm_so_request));

		  nm_so_sr_irecv(interface, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(interface, request);

		  nm_so_sr_isend(interface, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(interface, request);
		  }

		{
		  nm_so_request_t r1, r2, r3, r4;
		  r1=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r2=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r3=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r4=(nm_so_request_t) malloc(sizeof(nm_so_request));

		  char buf[16], *big_buf;
		  long gate;

		  nm_so_sr_irecv(interface, gate_id, 0, NULL, 0, &r1);
		  nm_so_sr_irecv(interface, gate_id, 0, NULL, 0, &r2);
		  nm_so_sr_irecv(interface, NM_SO_ANY_SRC, 0, buf, 16, &r3);

		  nm_so_sr_rwait(interface, r1);
		  printf("Got msg 1!\n");

		  nm_so_sr_rwait(interface, r2);
		  printf("Got msg 2!\n");

		  nm_so_sr_rwait(interface, r3);
		  nm_so_sr_recv_source(interface, r3, &gate);

		  printf("Got msg 3 : [%s] from gate %ld \n", buf, gate);

		  big_buf = malloc(MAX);
		  nm_so_sr_irecv(interface, NM_SO_ANY_SRC, 0, big_buf, MAX, &r4);
		  nm_so_sr_rwait(interface, r4);

		  printf("Got msg 4 : [%s]\n", big_buf);
		}

        } else {
	  int k;
                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv_id,
                                           hostname, r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        goto out;
                }

		for(k = 0; k < LOOPS; k++) {
			nm_so_request_t request=(nm_so_request_t) malloc(sizeof(nm_so_request));

		  nm_so_sr_isend(interface, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(interface, request);

		  nm_so_sr_irecv(interface, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(interface, request);
		}

		{
		  nm_so_request_t r1, r2, r3, r4;
		  r1=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r2=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r3=(nm_so_request_t) malloc(sizeof(nm_so_request));
		  r4=(nm_so_request_t) malloc(sizeof(nm_so_request));

		  char buf[16], *big_buf;


		  nm_so_sr_isend(interface, gate_id, 0, NULL, 0, &r1);
		  nm_so_sr_isend(interface, gate_id, 0, NULL, 0, &r2);

		  strcpy(buf, "Hello!");
		  nm_so_sr_isend(interface, gate_id, 0, buf, 16, &r3);

		  big_buf = malloc(MAX);
		  store_big_string(big_buf, MAX);

		  nm_so_sr_isend(interface, gate_id, 0, big_buf, MAX, &r4);

		  nm_so_sr_swait(interface, r1);
		  nm_so_sr_swait(interface, r2);
		  nm_so_sr_swait(interface, r3);
		  nm_so_sr_swait(interface, r4);
		}

        }

 out:
        return 0;
}
