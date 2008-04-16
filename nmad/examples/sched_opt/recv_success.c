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
#include <assert.h>

#include "helper.h"

int
main(int argc, char **argv) {
  struct nm_so_interface *sr_if;

  nm_so_init(&argc, argv);
  nm_so_get_sr_if(&sr_if);

  if(is_server()){
    //void *data = "rien";
    //nm_so_request request;
    //int ref;
    void *retour;

    //nm_so_sr_irecv_with_ref(sr_if, gate_id, 0,
    //                        data, strlen(data), &request, &ref);

    nm_so_sr_recv_success(sr_if, &retour);

    printf("Sortie\n");
  }

  nm_so_exit();
  exit(0);
}
