#include "marcel.h"
#ifdef MA__BUBBLES
static
any_t f(any_t foo) {
   int i = (int)(intptr_t)foo;
   int id = i/100;
   int load = i%100;
   int n;
   int sum = 0;
   marcel_printf("some work %d in %d\n",load,id);
   if (!load) marcel_delay(1000);
   for (n=0;n<load*10000000;n++) sum+=n;
   marcel_printf("%d done\n",id);
   return (void*)(intptr_t)sum;
}

marcel_bubble_t b0;
marcel_bubble_t b1;
marcel_bubble_t b2;
marcel_t t1;
marcel_t t2;
marcel_bubble_t b3;
marcel_t t3;
marcel_t t4;
marcel_t t5;
marcel_t t6;
marcel_bubble_t b4;
marcel_t t7;
marcel_t t8;
marcel_bubble_t b5;
marcel_t t9;
marcel_bubble_t b6;
marcel_t t10;
marcel_t t11;
marcel_t t12;
marcel_bubble_t b7;
marcel_t t13;

static
void
bubble_init(marcel_bubble_t *b_outer, marcel_bubble_t *b_inner) {
	marcel_bubble_init(b_inner);
	marcel_bubble_insertbubble(b_outer, b_inner);
}

static
void
thread_init(marcel_t *t, marcel_bubble_t *b, intptr_t arg) {
	marcel_attr_t attr;
	marcel_attr_init(&attr);
	marcel_attr_setnaturalbubble(&attr, b);
	marcel_attr_setid(&attr,0);
	marcel_attr_setprio(&attr, MA_DEF_PRIO);
	marcel_attr_setname(&attr,"thread");
	marcel_create(t, &attr, f, (any_t)arg);
	*marcel_stats_get(*t, load) = 10;
}

int main(int argc, char *argv[]) {

   marcel_init(&argc,argv);
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
   marcel_printf("started\n");

   //création de la bulle 0 :
   bubble_init(&marcel_root_bubble, &b0);
   //création de la bulle 1 :
   bubble_init(&b0, &b1);
   //création de la bulle 2 :
   bubble_init(&b1, &b2);
   //création du thread id = 1 :
   thread_init(&t1, &b2, 2);
   //création du thread id = 2 :
   thread_init(&t2, &b2, 2);
   //création de la bulle 3 :
   bubble_init(&b1, &b3);
   //création du thread id = 3 :
   thread_init(&t3, &b3, 50);
   //création du thread id = 4 :
   thread_init(&t4, &b3, 50);
   //création du thread id = 5 :
   thread_init(&t5, &b3, 50);
   //création du thread id = 6 :
   thread_init(&t6, &b3, 50);
   //création de la bulle 4 :
   bubble_init(&b1, &b4);
   //création du thread id = 7 :
   thread_init(&t7, &b4, 50);
   //création du thread id = 8 :
   thread_init(&t8, &b4, 50);
   //création de la bulle 5 :
   bubble_init(&b1, &b5);
   //création du thread id = 9 :
   thread_init(&t9, &b5, 50);
   //création de la bulle 6 :
   bubble_init(&b1, &b6);
   //création du thread id = 10 :
   thread_init(&t10, &b6, 50);
   //création du thread id = 11 :
   thread_init(&t11, &b6, 50);
   //création du thread id = 12 :
   thread_init(&t12, &b6, 50);
   //création de la bulle 7 :
   bubble_init(&b1, &b7);
   //création du thread id = 13 :
   thread_init(&t13, &b7, 50);

   marcel_start_playing();
   marcel_bubble_sched_begin();

   marcel_printf("ok\n");
   marcel_end();

#ifdef PROFILE
   profile_stop();
#endif
   return 0;
}
#else /* MA__BUBBLES */
int main(int argc, char *argv[]) {
   fprintf(stderr,"%s needs bubbles\n",argv[0]);
   return 0;
}
#endif /* MA__BUBBLES */
