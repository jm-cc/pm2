
/*
 * Mad_regular_spawn.h
 * ===================
 */

#ifndef MAD_REGULAR_SPAWN_H
#define MAD_REGULAR_SPAWN_H

/*
 * Fonctions exportees
 * -------------------
 */

void
mad_slave_spawn(p_mad_madeleine_t   madeleine,
		int                 argc,
		char              **argv);

p_mad_madeleine_t
mad_init(int                  *argc,
	 char                **argv,
	 char                 *configuration_file,
	 p_mad_adapter_set_t   adapter_set);

#endif /* MAD_REGULAR_SPAWN_H */
