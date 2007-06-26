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
#include <sys/uio.h>

/* Choose a scheduler
 */
#include <tbx.h>
#include <nm_public.h>
#include <nm_so_public.h>
#include "nm_so_pkt_wrap.h"


/* Choose a driver
 */
#include <nm_drivers.h>

#define TAG0 128
#define TAG1 129

static int data_handler(struct nm_so_pkt_wrap *p_so_pw, 
			void *ptr, uint32_t len,
			uint8_t proto_id, uint8_t seq)
{
  printf("Data header : data = %p [%s], len = %u, tag = %d, seq = %u\n",
	 ptr, (char *)ptr, len, proto_id, seq);

  return NM_SO_HEADER_MARK_UNREAD;
}

static int rdv_handler(struct nm_so_pkt_wrap *p_so_pw,
		       uint8_t tag_id, uint8_t seq, uint32_t len)
{
  printf("Rdv header : tag = %d, seq = %u, len = %u\n", tag_id, seq, len);

  return NM_SO_HEADER_MARK_UNREAD;
}

int
main(int	  argc,
     char	**argv)
{
  struct nm_core *p_core = NULL;
  int err;
  tbx_tick_t t1, t2;
  int k;

  err = nm_core_init(&argc, argv, &p_core, nm_so_load);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out;
  }

  for(k = 0; k < 5; k++)
  {
    struct nm_so_pkt_wrap *p_so_pw;
    NM_SO_ALIGN_TYPE buf[NM_SO_PREALLOC_BUF_LEN / sizeof(NM_SO_ALIGN_TYPE)];
    struct iovec *vec;
    void *ptr;
    uint32_t len;
    int i;

    TBX_GET_TICK(t1);

    err = nm_so_pw_alloc_and_fill_with_data(TAG0, 0,
					    "abc", 4,
					    NM_SO_DATA_USE_COPY,
					    &p_so_pw);
    if(err != NM_ESUCCESS) {
      printf("nm_so_pw_alloc failed: err = %d\n", err);
      goto out;
    }

    nm_so_pw_add_data(p_so_pw, TAG1, 0, "d", 2, NM_SO_DATA_FORCE_CONTIGUOUS);

    nm_so_pw_add_data(p_so_pw, TAG0, 1, "efgh", 5, 0);

    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, TAG0, 2, 4 + 2 + 5);

      nm_so_pw_add_control(p_so_pw, &ctrl);
    }

    nm_so_pw_finalize(p_so_pw);

    TBX_GET_TICK(t2);

    printf("Total length = %lld\n", p_so_pw->pw.length);
    for(i=0; i<p_so_pw->pw.v_nb; i++)
      printf("iovec[%d] contains %u bytes (@ = %p)\n",
	     i, p_so_pw->pw.v[i].iov_len, p_so_pw->pw.v[i].iov_base);

    printf("Iteration sur le wrapper d'emission :\n");
    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_handler,
				  rdv_handler,
				  NULL);


    vec = p_so_pw->pw.v;
    ptr = buf;
    for(i=0; i<p_so_pw->pw.v_nb; i++) {
      memcpy(ptr, vec->iov_base, vec->iov_len);
      ptr += vec->iov_len;
      vec++;
    }

    len = ptr - (void *)buf;
    printf("taille recopiée = %d\n", len);


    nm_so_pw_free(p_so_pw);

    err = nm_so_pw_alloc_and_fill_with_data(0, 0,
                                            buf, NM_SO_PREALLOC_BUF_LEN,
			                    NM_SO_DATA_DONT_USE_HEADER,
                                            &p_so_pw);
    if(err != NM_ESUCCESS) {
      printf("nm_so_pw_alloc failed: err = %d\n", err);
      goto out;
    }

    printf("Iteration sur le wrapper de reception :\n");
    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_handler,
				  rdv_handler,
				  NULL);

    nm_so_pw_free(p_so_pw);

  }


  printf("time = %lf\n", TBX_TIMING_DELAY(t1, t2));

 out:
        return err;
}

