
/*
 * ntbx_pack.c
 * -----------
 */
/* #define DEBUG */
#include "ntbx.h"
#include <stdlib.h>
#include <string.h>

/*
 * Integer
 * -------
 */
void
ntbx_pack_int(int                  data,
	      p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "I");
    }
  sprintf(ptr,
	  "%*d%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_VAL("data", data);
  LOG_STR("pack_buffer", pack_buffer);
  LOG_OUT();
}

int
ntbx_unpack_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'I')
	FAILURE("synchronisation error");
    }
  data = atoi(ptr);
  LOG_VAL("data", data);
  LOG_STR("pack_buffer", pack_buffer);
  LOG_OUT();
  return data;
}

/*
 * Long integer
 * ------------
 */
void
ntbx_pack_long(long                 data,
	       p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "L");
    }
  sprintf(ptr,
	  "%*ld%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_OUT();
}

long
ntbx_unpack_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  long  data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'L')
	FAILURE("synchronisation error");
    }
  data = atol(ptr);
  LOG_OUT();
  return data;
}

/*
 * Double
 * ------
 */
void
ntbx_pack_double(double               data,
		 p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      sprintf(ptr++, "D");
    }
  sprintf(ptr,
	  "%*f%c",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	  data, 0);
  LOG_OUT();
}

int
ntbx_unpack_double(p_ntbx_pack_buffer_t pack_buffer)
{
  void   *ptr = pack_buffer->buffer;
  double  data;
  int     i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'D')
	FAILURE("synchronisation error");
    }
  data = atof(ptr);
  LOG_OUT();
  return data;
}
