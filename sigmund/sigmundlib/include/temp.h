#ifndef TEMP_H
#define TEMP_H

#include "sigmund.h"
#include "fkt.h"
#include "fkt_names.h"

struct code_name	fkt_code_table[] =
{
#include "fkt_code_name.h"
  {0, NULL}
};


char	*sys_irqs[NSYS_IRQS] =	{
#include "fkt_sysirq_name.h"
};


char	*traps[NTRAPS+1] =	{
#include "fkt_traps_name.h"
};


char	*sys_calls[NSYS_CALLS+1] = {
#include "fkt_syscall_name.h"
};

#endif
