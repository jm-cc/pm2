
/*
 * Mad_configuration.h
 * ===================
 */

#ifndef MAD_CONFIGURATION_H
#define MAD_CONFIGURATION_H

/*
 * Madeleine configuration
 * -----------------------
 */
typedef struct s_mad_configuration
{
  mad_configuration_size_t   size;
  ntbx_host_id_t             local_host_id;
  char                     **host_name; /* configuration host name list */
  char                     **program_name; /* program name list */
} mad_configuration_t;

#endif /* MAD_CONFIGURATION_H */
