
/*
 * Mad_application_spawn.h
 * =======================
 */

#ifndef MAD_APPLICATION_SPAWN_H
#define MAD_APPLICATION_SPAWN_H

/*
 * Controles du mode de compilation
 * --------------------------------
 */
#ifdef PM2
#error APPLICATION_SPAWN is not yet supported with PM2
#endif /* PM2 */

#ifdef EXTERNAL_SPAWN
#error EXTERNAL_SPAWN cannot be specified with APPLICATION_SPAWN
#endif /* EXTERNAL_SPAWN */

/*
 * Fonctions exportees
 * -------------------
 */
char *
mad_pre_init(p_mad_adapter_set_t adapter_set);

p_mad_madeleine_t
mad_init(ntbx_host_id_t  rank,
	 char           *configuration_file __attribute__ ((unused)),
	 char           *url);

#endif /* MAD_APPLICATION_SPAWN_H */
