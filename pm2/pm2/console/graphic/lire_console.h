

/* included by xpm2.c */
#ifndef __lire_console_h__
#define __lire_console_h__

#include "structures.h"

void       extract_word (char *buf, char nom[128], int *i);
p_host     extract_conf (char *buf, p_host previous_hosts);
p_pvm_task extract_ps   (char *buf, p_host ph, p_pvm_task previous_pvm_tasks);
p_thread   extract_th   (char *buf, p_pvm_task ppt, p_thread previous_threads);

#endif /* __lire_console_h__ */

