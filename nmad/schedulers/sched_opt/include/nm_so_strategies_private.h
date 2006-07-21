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

#ifndef NM_SO_STRATEGIES_PRIVATE_H
#define NM_SO_STRATEGIES_PRIVATE_H

#define PENDING_PW_WINDOW_SIZE 10

struct nm_so_se_shorter_pw{
    int index;

    // info discriminante
    int len;
    int nb_seg;

    struct nm_pkt_wrap *src_pw;
};

struct nm_so_se_pending_tab{
    struct nm_so_se_shorter_pw *tab[PENDING_PW_WINDOW_SIZE];
    int nb_pending_pw;
};

enum nm_so_se_operation_type{
    nm_so_se_aggregation,
    nm_so_se_split
};

struct nm_so_se_operation{
    int operation_type;

    int pw_idx[PENDING_PW_WINDOW_SIZE];
    int nb_pw;

    int nb_seg;

    int altitude;
};

struct nm_so_se_op_stack{
    struct nm_so_stack *op_stack;
    double score;
};

#endif
