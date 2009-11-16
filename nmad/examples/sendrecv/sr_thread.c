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

#include <stdio.h>
#include <pthread.h>
#include "helper.h"

#define N 500000

void *server(void* foo) {
	char buf[128];
	int i = 0;

	nm_sr_request_t request;
	for (i = 0; i < N; i++) {
		nm_sr_irecv(p_core, gate_id, 0, buf, sizeof(buf), &request);
		nm_sr_rwait(p_core, &request);
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	int i = 0;

	init(&argc, argv);

#ifdef MARCEL
	marcel_t t;
	marcel_create(&t, NULL, server, NULL);
#else
	pthread_t t;
	pthread_create(&t, NULL, server, NULL);
#endif
	for (i = 0; i < N; i++) {
		char buf[128];
		nm_sr_request_t request;
		//usleep(1);
		nm_sr_isend(p_core, gate_id, 0, buf, sizeof(buf), &request);
		nm_sr_swait(p_core, &request);
		if (i%(N/50) == 0)
			printf("%d\n",i);
	}
#ifdef MARCEL
	marcel_join(t, NULL);
#else
	pthread_join(t, NULL);
#endif

	nmad_exit();
	return 0;
}
