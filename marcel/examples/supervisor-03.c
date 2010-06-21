#include "marcel.h"

static
void *
f (void *_arg TBX_UNUSED) {
	int i;
	marcel_printf ("current vp = %u\n", marcel_current_vp ());
	for (i = 0; i < 8; i++) {
		unsigned long start = marcel_clock();
		while(marcel_clock() < start + 1000);
		marcel_printf ("current vp = %u\n", marcel_current_vp ());
	}
	marcel_printf ("done\n");
	return NULL;
}

int
main (int argc, char ** argv) {
	marcel_t t;
	marcel_fprintf (stderr, "supervisor test\n");
	marcel_init(argc, argv);
	marcel_supervisor_init ();
	marcel_fprintf (stderr, "creating user thread\n");
	marcel_create (&t, NULL, f, NULL);
	marcel_join (t, NULL);
	marcel_supervisor_sync ();
	marcel_end ();
	return 0;
}




