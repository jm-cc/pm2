
/*
 * Mad_exit.c
 * ==========
 */
/* #define TIMING */
#include "madeleine.h"

static void
mad_foreach_adapter_exit(void *object)
{
  p_mad_adapter_t          adapter   = object;
  p_mad_driver_t           driver    = adapter->driver;
  p_mad_driver_interface_t interface = &(driver->interface);
  
  LOG_IN();
  if (interface->adapter_exit)
    {
      interface->adapter_exit(adapter);
    }
  else
    {
      if (adapter->specific)
	{
	  TBX_FREE(adapter->specific);
	}
    }
  LOG_OUT();
}

static void
mad_driver_exit(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t           driver;
      p_mad_driver_interface_t interface;
      
      driver    = &(madeleine->driver[drv]);      
      interface = &(driver->interface);

      tbx_foreach_destroy_list(&(madeleine->channel),
			       mad_foreach_adapter_exit);
      if (interface->driver_exit)
	{
	  interface->driver_exit(driver);
	}
      else
	{
	  if (driver->specific)
	    {
	      TBX_FREE(driver->specific);
	    }
	}
    }

  TBX_FREE(madeleine->adapter);
  TBX_FREE(madeleine->driver);
  LOG_OUT();
}

static void
mad_settings_exit(p_mad_madeleine_t madeleine)
{
  p_mad_settings_t settings = madeleine->settings;

  LOG_IN();
  settings->rsh_cmd = NULL;
  if (settings->configuration_file)
    {
      TBX_FREE(settings->configuration_file);
      settings->configuration_file = NULL;
    }
  settings->debug_mode            = tbx_false;
  settings->external_spawn_driver =        -1;
  LOG_OUT();
}

static void
mad_configuration_exit(p_mad_madeleine_t madeleine)
{
  p_mad_configuration_t configuration = madeleine->configuration;

  LOG_IN();
  if (configuration->program_name)
    {
      ntbx_host_id_t id = configuration->local_host_id;
      
      while (id--)
	{
	  if (configuration->program_name[id])
	    {
	      TBX_FREE(configuration->program_name[id]);
	      configuration->program_name[id] = NULL;
	    }
	}

      TBX_FREE(configuration->program_name);
      configuration->program_name = NULL;
    }
  
  if (configuration->host_name)
    {
      ntbx_host_id_t id = configuration->local_host_id;
      
      while (id--)
	{
	  if (configuration->host_name[id])
	    {
	      TBX_FREE(configuration->host_name[id]);
	      configuration->host_name[id] = NULL;
	    }
	}

      TBX_FREE(configuration->host_name);
      configuration->host_name = NULL;
    }
  
  configuration->local_host_id = -1;
  configuration->size          =  0;
  LOG_OUT();
}

void
mad_exit(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  TBX_LOCK_SHARED(madeleine);
  mad_close_channels(madeleine);
  mad_driver_exit(madeleine);
  tbx_list_manager_exit();

  if (madeleine->settings)
    {
      mad_settings_exit(madeleine);
      TBX_FREE(madeleine->settings);
      madeleine->settings = NULL;
    }
  
  if (madeleine->configuration)
    {
      mad_configuration_exit(madeleine);
      TBX_FREE(madeleine->configuration);
      madeleine->configuration = NULL;
    }

  TBX_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}


