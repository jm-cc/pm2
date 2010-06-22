#include "marcel.h"

static
void *
f (void *_arg TBX_UNUSED) {
	int i;

	marcel_printf ("%p:, current vp = %u\n", marcel_self(), marcel_current_vp ());
	for (i = 0; i < 20; i++) {
		unsigned long start = marcel_clock();
		while(marcel_clock() < start + 1000);
		marcel_printf ("%p: current vp = %u\n", marcel_self(), marcel_current_vp ());
	}
	marcel_printf ("%p: done\n", marcel_self());
	return NULL;
}

int
main (int argc, char ** argv) {
	marcel_t t1,t2;
	marcel_fprintf (stderr, "supervisor test\n");
	marcel_init(&argc, argv);
	marcel_supervisor_init ();
	marcel_fprintf (stderr, "nb vps = %d\n", marcel_nbvps());
	marcel_fprintf (stderr, "creating user thread\n");
	marcel_create (&t1, NULL, f, NULL);
	marcel_create (&t2, NULL, f, NULL);
	marcel_supervisor_sync ();
	marcel_join (t1, NULL);
	marcel_join (t2, NULL);
	marcel_create (&t1, NULL, f, NULL);
	marcel_create (&t2, NULL, f, NULL);
	marcel_supervisor_sync ();
	marcel_join (t1, NULL);
	marcel_join (t2, NULL);
	marcel_end ();
	return 0;
}




