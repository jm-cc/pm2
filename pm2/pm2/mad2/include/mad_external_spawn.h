
/*
 * Mad_external_spawn.h
 * ====================
 */

#ifndef MAD_EXTERNAL_SPAWN_H
#define MAD_EXTERNAL_SPAWN_H

/*
 * Controles du mode de compilation
 * --------------------------------
 */
#ifdef APPLICATION_SPAWN
#error APPLICATION_SPAWN cannot be specified with EXTERNAL_SPAWN
#endif /* APPLICATION_SPAWN */

/*
 * Fonctions exportees
 * -------------------
 */

p_mad_madeleine_t
mad_init(int                  *argc,
	 char                **argv,
	 char                 *configuration_file,
	 p_mad_adapter_set_t   adapter_set);

void
mad_spawn_driver_init(p_mad_madeleine_t   madeleine,
		      int                *argc,
		      char              **argv);

void
mad_exchange_connection_info(p_mad_madeleine_t madeleine);

#endif /* MAD_EXTERNAL_SPAWN_H */
