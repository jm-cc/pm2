
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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
#include "tsp.h"

void init_queue(struct s_tsp_queue *q)
{
	q->first = 0;
	q->last = 0;
	MUTEX_INIT(&q->mutex, NULL);
}

void add_job(struct s_tsp_queue *q, struct s_job j)
{
	struct s_maillon *ptr;
	unsigned int i;

	ptr = malloc(sizeof(struct s_maillon));
	ptr->next = 0;
	ptr->tsp_job.len = j.len;
	for (i = 0; i < MAXE; i++)
		ptr->tsp_job.path[i] = j.path[i];

	MUTEX_LOCK(&q->mutex);
	if (q->first == 0)
		q->first = q->last = ptr;
	else {
		q->last->next = ptr;
		q->last = ptr;
	}
	MUTEX_UNLOCK(&q->mutex);
}

int get_job(struct s_tsp_queue *q, struct s_job *j)
{
	struct s_maillon *ptr;
	unsigned int i;

	MUTEX_LOCK(&q->mutex);
	if (q->first == 0) {
		MUTEX_UNLOCK(&q->mutex);
		return 0;
	}

	ptr = q->first;

	q->first = ptr->next;
	if (q->first == 0)
		q->last = 0;
	MUTEX_UNLOCK(&q->mutex);

	j->len = ptr->tsp_job.len;
	for (i = 0; i < MAXE; i++)
		j->path[i] = ptr->tsp_job.path[i];

	free(ptr);
	return 1;

}
