#include "stdio.h"

unsigned long _dsm_get_page_size()
{
#if defined(SOLARIS_SYS) || defined(IRIX_SYS)
  return sysconf(_SC_PAGESIZE);
#elif defined(UNICOS_SYS)
  return 1024;
#else
  return getpagesize();
#endif
}

int main()
{
 fprintf(stdout, "%d", _dsm_get_page_size());
}
