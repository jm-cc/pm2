
/*
 * Mad_main.h
 * ===========
 */

#ifndef MAD_MAIN_H
#define MAD_MAIN_H

/*
 * Structures
 * ----------
 */

/*
 * Connection registration array
 * -----------------------------
 */

typedef struct s_mad_settings
{
  char            *rsh_cmd;
  char            *configuration_file;
  tbx_bool_t       debug_mode;
  mad_driver_id_t  external_spawn_driver;
#ifdef MAD_APPLICATION_SPAWN
  char            *url;
#endif /* MAD_APPLICATION_SPAWN */
} mad_settings_t;


typedef struct s_mad_madeleine
{
  TBX_SHARED;
  mad_driver_id_t       nb_driver;
  p_mad_driver_t        driver;
  mad_adapter_id_t      nb_adapter;
  p_mad_adapter_t       adapter;
  mad_channel_id_t      nb_channel;
  tbx_list_t            channel;
  p_mad_settings_t      settings;
  p_mad_configuration_t configuration;
#ifdef LEONIE_SPAWN
  p_ntbx_client_t       master_link;
#endif /* LEONIE_SPAWN */
} mad_madeleine_t;

/*
 * Functions 
 * ---------
 */
int
mad_protocol_available(p_mad_madeleine_t  madeleine,
		       char              *name);

p_mad_adapter_set_t
mad_adapter_set_init(int nb_adapter, ...);

/*
 * Private part : these functions should not be called directly
 * --------------...............................................
 */

void 
mad_driver_fill(p_mad_madeleine_t madeleine);

void 
mad_driver_init(p_mad_madeleine_t madeleine);

void 
mad_adapter_fill(p_mad_madeleine_t     madeleine,
		 p_mad_adapter_set_t   adapter_set);

void
mad_adapter_init(p_mad_madeleine_t madeleine);

void
mad_adapter_configuration_init(p_mad_madeleine_t madeleine);

char *
mad_get_mad_root(void);

p_mad_madeleine_t
mad_object_init(int                   argc,
		char                **argv,
		char                 *configuration_file,
		p_mad_adapter_set_t   adapter_set);

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv);

void
mad_configuration_init(p_mad_madeleine_t   madeleine,
		       int                 argc,
		       char              **argv);

void
mad_output_redirection_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv);

void
mad_network_components_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv);

void  
mad_connect(p_mad_madeleine_t   madeleine,
	    int                 argc,
	    char              **argv);

void
mad_purge_command_line(p_mad_madeleine_t   madeleine,
		       int                *_argc,
		       char              **_argv);

#endif /* MAD_MAIN_H */
