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
 * Mad_pmi.c
 * =========
 */

#ifdef MAD3_PMI
#include <pmi.h>
#include "madeleine.h"

void
mad_pmi_init(p_mad_madeleine_t madeleine) {
        p_mad_pmi_t	pmi		= NULL;
	int		pmi_errno	= PMI_SUCCESS;

        pmi = mad_pmi_cons();

	pmi_errno	= PMI_Init(&pmi->has_parent);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_has_parent",	pmi->has_parent);

	pmi_errno	= PMI_Get_rank(&pmi->rank);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_rank",	pmi->rank);

	pmi_errno	= PMI_Get_size(&pmi->size);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_size",	pmi->size);

	pmi_errno	= PMI_Get_appnum(&pmi->appnum);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_appnum",	pmi->appnum);

	pmi_errno	= PMI_Get_id_length_max(&pmi->id_length_max);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_id_length",	pmi->id_length_max);

	pmi->id	= TBX_MALLOC(pmi->id_length_max + 1);
	pmi_errno	= PMI_Get_id(pmi->id, pmi->id_length_max);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_STR("pmi_id",	pmi->id);

	pmi_errno	= PMI_KVS_Get_name_length_max(&pmi->kvs_name_length_max);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_kvs_name_length_max",	pmi->kvs_name_length_max);

        pmi->kvs_name	= TBX_MALLOC(pmi->kvs_name_length_max + 1);
        pmi_errno	= PMI_KVS_Get_my_name(pmi->kvs_name, pmi->kvs_name_length_max);
        TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_STR("pmi_kvs_name", pmi->kvs_name);

	pmi_errno	= PMI_KVS_Get_key_length_max(&pmi->kvs_key_length_max);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_kvs_key_length_max",	pmi->kvs_key_length_max);

	pmi_errno	= PMI_KVS_Get_value_length_max(&pmi->kvs_val_length_max);
	TBX_ASSERT(pmi_errno == PMI_SUCCESS);

        DISP_VAL("pmi_kvs_val_length_max",	pmi->kvs_val_length_max);

        madeleine->pmi	= pmi;
}

void
mad_pmi_directory_init(p_mad_madeleine_t madeleine) {
        p_mad_pmi_t pmi	= NULL;

        LOG_IN();
        pmi	= madeleine->pmi;
        LOG_OUT();
}

void
mad_pmi_drivers_init(p_mad_madeleine_t madeleine) {
        p_mad_pmi_t pmi = NULL;

        LOG_IN();
        pmi	= madeleine->pmi;
        LOG_OUT();
}

void
mad_pmi_channels_init(p_mad_madeleine_t madeleine) {
        p_mad_pmi_t pmi = NULL;

        LOG_IN();
        pmi	= madeleine->pmi;
        LOG_OUT();
}


#endif  /* MAD3_PMI */
