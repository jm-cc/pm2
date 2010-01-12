#include "marcel.h"

static
void *
f (void *_arg TBX_UNUSED) {
	int i;
	marcel_printf ("current vp = %u\n", marcel_current_vp ());
	for (i = 0; i < 20; i++) {
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
	marcel_attr_t attr;
	marcel_vpset_t vpset = MARCEL_VPSET_ZERO;
	marcel_fprintf (stderr, "supervisor test\n");
	marcel_init (&argc, argv);
	marcel_supervisor_init ();
	marcel_fprintf (stderr, "creating user thread\n");
	marcel_attr_init (&attr);
	marcel_vpset_set (&vpset, 2);
	marcel_vpset_set (&vpset, 3);
	marcel_attr_setvpset (&attr, vpset);
	marcel_create (&t, &attr, f, NULL);
	marcel_join (t, NULL);
	marcel_end ();
	return 0;
}




