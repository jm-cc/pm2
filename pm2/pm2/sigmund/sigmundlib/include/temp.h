#ifndef TEMP_H
#define TEMP_H

#include "sigmund.h"
#include "fkt.h"

struct code_name	fkt_code_table[] =
{
#include "fkt_code_name.h"
  
  {0, NULL}
};

#define	NTRAPS	20

char	*traps[NTRAPS+1] =	{
#include "fkt_traps_name.h"
};


#define NSYS_CALLS		232

char	*sys_calls[NSYS_CALLS+1] = {
#include "fkt_syscall_name.h"
};

#endif
