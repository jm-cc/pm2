
#ifndef DSM_PM2_IS_DEF
#define DSM_PM2_IS_DEF

#include "dsm_const.h" 
#include "dsm_sysdep.h"

/* the following 2 need to be included by the user
 * program to allow:
 *  - protocol initialization using library functions
 *  - user-defined protocol epecification
 */

#include "dsm_protocol_lib.h" 
#include "dsm_protocol_policy.h"
#include "dsm_lock.h"

/* to enable the user to configure the distribution of dsm pages */
#include "dsm_page_manager.h"

/* to enable the user to use dsm mutex */
#include "dsm_mutex.h"

#define BEGIN_DSM_DATA \
  asm (".data"); \
  asm (DSM_ASM_PAGEALIGN); \
  char dsm_data_begin = {}; 

#define END_DSM_DATA \
  char dsm_data_end = {}; \
  asm (DSM_ASM_PAGEALIGN);

#define DSM_NEWPAGE \
  asm (DSM_ASM_PAGEALIGN);

void dsm_pm2_init(int my_rank, int confsize);

void dsm_pm2_exit();

#endif




