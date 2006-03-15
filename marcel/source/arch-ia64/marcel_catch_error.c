
#include "marcel.h"

unsigned long __ma_bad_increment_for_ia64_fetch_and_add(void)
{
	MA_BUG();
	return 0;
}

