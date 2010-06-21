#include "marcel.h"

any_t f(any_t foo) {
	int i = (int)foo;
	marcel_printf("some work in %d\n",i);
	return NULL;
}

marcel_bubble_t b1, b2, b;
marcel_attr_t attr1, attr2, attr;
marcel_t t1, t2, t3, t4, t5;

int main(int argc, char *argv[]) {
	marcel_init(argc,argv);
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
	marcel_printf("started\n");

	marcel_bubble_init(&b1);

	marcel_attr_init(&attr1);
	marcel_attr_setinitbubble(&attr1, &b1);
	marcel_create(&t1, &attr1, f, (any_t)1);
	marcel_create(&t2, &attr1, f, (any_t)2);

	marcel_bubble_init(&b2);

	marcel_attr_init(&attr2);
	marcel_attr_setinitbubble(&attr2, &b2);
	marcel_create(&t3, &attr2, f, (any_t)3);
	marcel_create(&t4, &attr2, f, (any_t)4);

	marcel_bubble_init(&b);
	marcel_bubble_insertbubble(&b, &b1);
	marcel_bubble_insertbubble(&b, &b2);

	marcel_attr_init(&attr);
	marcel_attr_setinitbubble(&attr, &b);
	marcel_create(&t5, &attr, f, (any_t)5);

	marcel_wake_up_bubble(&b);
	
	marcel_printf("ok\n");
	profile_stop();
	marcel_end();
	return 0;
}
