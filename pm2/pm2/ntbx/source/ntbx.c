
/*
 * Ntbx.c
 * ======
 */

#include "tbx.h"
#include "ntbx.h"

static volatile tbx_bool_t initialized = tbx_false;

void
ntbx_init(int    argc,
	  char **argv)
{
  LOG_IN();
  if (!initialized)
    {
      initialized = tbx_true;
    }
  LOG_OUT();

}

void
ntbx_purge_cmd_line(int   *argc,
		    char **argv)
{
  LOG_IN();
  /* --- */
  LOG_OUT();
}
