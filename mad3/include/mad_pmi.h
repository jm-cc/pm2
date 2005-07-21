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

/*
 * Mad_pmi.h
 * =========
 */

#ifndef MAD_PMI_H
#define MAD_PMI_H

typedef struct s_mad_pmi {
        int	has_parent;
        int	rank;
        int	size;
        int	appnum;
        int	id_length_max;
        char	*id;
        int	kvs_name_length_max;
        char	*kvs_name;
        int	kvs_key_length_max;
        int	kvs_val_length_max;
} mad_pmi_t, *p_mad_pmi_t;

#endif /* MAD_PMI_H */
