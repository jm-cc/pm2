
/*
 * Mad_adapter.h
 * ==============
 */

#ifndef MAD_ADAPTER_H
#define MAD_ADAPTER_H

typedef struct s_mad_adapter
{
  /* Common use fields */
  TBX_SHARED;
  p_mad_driver_t            driver;
  mad_adapter_id_t          id; 
  char                     *name;
  char                     *master_parameter;
  char                     *parameter;
  p_mad_driver_specific_t   specific; 
} mad_adapter_t;

#endif /* MAD_ADAPTER_H */
