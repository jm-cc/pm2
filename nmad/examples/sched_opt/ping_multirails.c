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

#include <nm_so_pack_interface.h>

#if defined CONFIG_MX && defined CONFIG_QSNET
#  include <nm_mx_public.h>
#  include <nm_qsnet_public.h>

//#define ISO_SPLIT

#define SPLIT_THRESHOLD (64 * 1024)
#define NB_PACKS      2
#define MIN           (4 * NB_PACKS)
#define MAX           (8 * 1024 * 1024)
#define LOOPS         2000

static __inline__
uint32_t _next(uint32_t len)
{
        if(!len)
                return 4;
        else if(len < 32)
                return len + 4;
        else if(len < 1024)
                  return len + 32;
        else
                return len << 1;
}

static __inline__
uint32_t _size(uint32_t len, unsigned n)
{
  unsigned chunk = len / NB_PACKS;

#ifndef ISO_SPLIT
  if (len >= 256 * 1024) {
    if(n & 1) { /* pack 1 */
      return chunk - chunk / 8;
    } else { /* pack 0 */
      return chunk + chunk / 8;
    }
  }
#endif

  return chunk;
}

static
void
usage(void) {
        fprintf(stderr, "usage: hello [[-h <remote_hostname>] <remote url> <remote url2>]\n");
        exit(EXIT_FAILURE);
}

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        char			*r_url1		= NULL;
        char			*r_url2		= NULL;
        char			*l_url[2]		= {NULL, NULL};
        uint8_t			drv_id[2]	=    {0, 0};
	int (*drv_load[2])(struct nm_drv_ops *) = { &nm_mx_load, &nm_qsnet_load };
        uint8_t			 gate_id	=    0;
        char			*buf		= NULL;
        char			*hostname	= "localhost";
        uint32_t		 len;
	struct nm_so_cnx         cnx;
        nm_so_pack_interface     interface;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

	err = nm_so_pack_interface_init(p_core, &interface);
	if(err != NM_ESUCCESS) {
                printf("nm_so_pack_interface_init return err = %d\n", err);
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
                                r_url1	= r_url2;
                                r_url2	= *argv;
                        }

                        argc--;
                        argv++;
                }

                printf("running as client using remote url: %s [%s] [%s]\n", hostname, r_url1, r_url2);
        } else {
                /* no args: server */
                printf("running as server\n");
        }


        err = nm_core_driver_load_init_some(p_core, 2, drv_load, drv_id, l_url);
        printf("local url1: [%s]\n", l_url[0]);
        printf("local url2: [%s]\n", l_url[1]);

        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_load_init returned err = %d\n", err);
                goto out;
        }

        err = nm_core_gate_init(p_core, &gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                goto out;
        }

        buf = malloc(MAX);
	memset(buf, 0, MAX);

        if (!r_url1 && !r_url2) {
                int k;
                /* server
                 */

                err = nm_core_gate_accept(p_core, gate_id, drv_id[0], NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_accept(p_core, gate_id, drv_id[1], NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv2) returned err = %d\n", err);
                        goto out;
                }

		for(len = MIN; len <= MAX; len = _next(len)) {
		  unsigned long n;
		  void *_buf;

		  for(k = 0; k < LOOPS; k++) {

		    _buf = buf;
		    nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
		    if(len < SPLIT_THRESHOLD)
		      nm_so_unpack(&cnx, _buf, len);
		    else
		      for(n = 0; n < NB_PACKS; n++) {
			unsigned long size = _size(len, n);
			nm_so_unpack(&cnx, _buf, size);
			_buf += size;
		      }
		    nm_so_end_unpacking(&cnx);

		    _buf = buf;
		    nm_so_begin_packing(interface, gate_id, 0, &cnx);
		    if(len < SPLIT_THRESHOLD)
		      nm_so_pack(&cnx, _buf, len);
		    else
		      for(n = 0; n < NB_PACKS; n++) {
			unsigned long size = _size(len, n);
			nm_so_pack(&cnx, _buf, size);
			_buf += size;
		      }
		    nm_so_end_packing(&cnx);
		  }
		}

        } else {
                tbx_tick_t t1, t2;
                int k;

                if (!r_url1)
                        usage();


                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv_id[0],
                                           r_url1);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_connect(p_core, gate_id, drv_id[1],
                                           r_url2);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv2) returned err = %d\n", err);
                        goto out;
                }

		for(len = MIN; len <= MAX; len = _next(len)) {
		  unsigned long n;
		  void *_buf;
		  double sum, lat, bw_million_byte, bw_mbyte;

		  TBX_GET_TICK(t1);

		  for(k = 0; k < LOOPS; k++) {

		    _buf = buf;
		    nm_so_begin_packing(interface, gate_id, 0, &cnx);
		    if(len < SPLIT_THRESHOLD)
		      nm_so_pack(&cnx, _buf, len);
		    else
		      for(n = 0; n < NB_PACKS; n++) {
			unsigned long size = _size(len, n);
			nm_so_pack(&cnx, _buf, size);
			_buf += size;
		      }
		    nm_so_end_packing(&cnx);

		    _buf = buf;
		    nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
		    if(len < SPLIT_THRESHOLD)
		      nm_so_unpack(&cnx, _buf, len);
		    else
		      for(n = 0; n < NB_PACKS; n++) {
			unsigned long size = _size(len, n);
			nm_so_unpack(&cnx, _buf, size);
			_buf += size;
		      }
		    nm_so_end_unpacking(&cnx);

		  }

		  TBX_GET_TICK(t2);

		  sum = TBX_TIMING_DELAY(t1, t2);
		  lat = sum / (2 * LOOPS);
		  bw_million_byte = len * (LOOPS / (sum / 2));
		  bw_mbyte = bw_million_byte / 1.048576;

		  printf("%d\t%lf\t%8.3f\t%8.3f\n",
		         len, lat, bw_million_byte, bw_mbyte);
		}
        }

 out:
        return 0;
}

#else
int main() {
        printf("This program requires Myrinet/MX and Quadrics networks subsystems\n");

        return 0;
}
#endif
