
#include "marcel.h"

#ifdef MA__LWPS

MA_DEFINE_PER_LWP(unsigned long *, ma_ia64_tp, NULL);

#endif

unsigned long __ma_bad_increment_for_ia64_fetch_and_add(void)
{
	MA_BUG();
	return 0;
}

