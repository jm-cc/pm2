
/*
 * Mad_adapter_description.h
 * ==================
 */

#ifndef MAD_ADAPTER_DESCRIPTION_H
#define MAD_ADAPTER_DESCRIPTION_H

typedef struct s_mad_adapter_description
{
  mad_driver_id_t   driver_id;
  char             *adapter_selector;
} mad_adapter_description_t;

typedef struct s_mad_adapter_set
{
  int                           size;
  p_mad_adapter_description_t   description;
} mad_adapter_set_t;

#endif /* MAD_ADAPTER_DESCRIPTION_H */
