
/*
 * Tbx.c
 * =====
 */
#include "tbx.h"

static volatile tbx_bool_t initialized = tbx_false;

void
tbx_init(int    argc,
	 char **argv)
{
  if (!initialized)
    {
      initialized = tbx_true;

      /* Safe malloc */
      //#ifdef TBX_SAFE_MALLOC
      tbx_safe_malloc_init();
      //#endif /* TBX_SAFE_MALLOC */

      /* Timer */
      tbx_timing_init();

      /* List manager */
      tbx_list_manager_init();

      /* Slist manager */
      tbx_slist_manager_init();

      /* Hash table manager */
      tbx_htable_manager_init();
    }
  
}

void
tbx_purge_cmd_line(int   *argc,
		   char **argv)
{
  LOG_IN();
  /* --- */
  LOG_OUT();
}
