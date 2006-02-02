#include "marcel.h"
int main(int argc, char *argv[]) {
	marcel_init(&argc,argv);
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
	marcel_printf("started\n");
	/* ... */
	marcel_printf("ok\n");
	profile_stop();
	marcel_end();
	return 0;
}
