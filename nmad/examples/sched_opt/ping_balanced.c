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

#define MIN     8
#define MAX     (1024 * 1024)
#define LOOPS   2000

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
unsigned long subpart_for_mx(unsigned long len)
{
  if(len > 32 * 1024) {
    return nm_so_aligned((len * 100 / 473) - 7000);
  } else
    return len / 2;
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
        char			*l_url1		= NULL;
        char			*l_url2		= NULL;
        uint8_t			 drv1_id	=    0;
        uint8_t			 drv2_id	=    0;
        uint8_t			 gate_id	=    0;
        char			*buf		= NULL;
        char			*hostname	= "localhost";
        uint32_t		 len;
	struct nm_so_cnx        *cnx            = NULL;
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                printf("nm_core_init returned err = %d\n", err);
                goto out;
        }

	err = nm_so_pack_interface_init();
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

        err = nm_core_driver_init(p_core, nm_mx_load, &drv1_id, &l_url1);
        printf("local url1: [%s]\n", l_url1);
        err = nm_core_driver_init(p_core, nm_qsnet_load, &drv2_id, &l_url2);
        printf("local url2: [%s]\n", l_url2);

        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_init returned err = %d\n", err);
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

                err = nm_core_gate_accept(p_core, gate_id, drv1_id, NULL, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_accept(p_core, gate_id, drv2_id, NULL, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept(drv2) returned err = %d\n", err);
                        goto out;
                }

		for(len = MIN; len <= MAX; len = _next(len)) {
		  unsigned long for_mx = subpart_for_mx(len);
		  unsigned long for_qs = len - for_mx;

                        for(k = 0; k < LOOPS; k++) {
                                nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
                                nm_so_unpack(cnx, buf, for_mx);
                                nm_so_unpack(cnx, buf + for_mx, for_qs);
                                nm_so_end_unpacking(p_core, cnx);

                                nm_so_begin_packing(p_core, gate_id, 0, &cnx);
                                nm_so_pack(cnx, buf, for_mx);
                                nm_so_pack(cnx, buf + for_mx, for_qs);
                                nm_so_end_packing(p_core, cnx);
                        }
		}

        } else {
                tbx_tick_t t1, t2;
                int k;

                if (!r_url1)
                        usage();
          

                /* client
                 */
                err = nm_core_gate_connect(p_core, gate_id, drv1_id,
                                           hostname, r_url1);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv1) returned err = %d\n", err);
                        goto out;
                }

                err = nm_core_gate_connect(p_core, gate_id, drv2_id,
                                           hostname, r_url2);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect(drv2) returned err = %d\n", err);
                        goto out;
                }

		for(len = MIN; len <= MAX; len = _next(len)) {
		  unsigned long for_mx = subpart_for_mx(len);
		  unsigned long for_qs = len - for_mx;
		  /*
		  printf("Sending %d bytes (%d over MX, %d over QSNET)\n",
			 len, for_mx, for_qs);
		  */
                        TBX_GET_TICK(t1);

                        for(k = 0; k < LOOPS; k++) {
                                nm_so_begin_packing(p_core, gate_id, 0, &cnx);
                                nm_so_pack(cnx, buf, for_mx);
                                nm_so_pack(cnx, buf + for_mx, for_qs);
                                nm_so_end_packing(p_core, cnx);

                                nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
                                nm_so_unpack(cnx, buf, for_mx);
                                nm_so_unpack(cnx, buf + for_mx, for_qs);
                                nm_so_end_unpacking(p_core, cnx);
                        }

                        TBX_GET_TICK(t2);

                        printf("%d\t%lf\n", len, TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));
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
