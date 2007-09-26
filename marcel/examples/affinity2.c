#include "marcel.h"
#ifdef MA__BUBBLES
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

int main(int argc, char *argv[]) {

   marcel_init(&argc,argv);
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
   marcel_printf("started\n");

   //création de la bulle 0 :
   marcel_bubble_init(&b0);
   //création de la bulle 1 :
   marcel_bubble_init(&b1);
   marcel_bubble_insertbubble(&b0, &b1);
   //création de la bulle 2 :
   marcel_bubble_init(&b2);
   marcel_bubble_insertbubble(&b1, &b2);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t1, &attr, f, (any_t)2);
      *marcel_stats_get(t1, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t2, &attr, f, (any_t)2);
      *marcel_stats_get(t2, load) = 10;
   }

   //création de la bulle 3 :
   marcel_bubble_init(&b3);
   marcel_bubble_insertbubble(&b1, &b3);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t3, &attr, f, (any_t)50);
      *marcel_stats_get(t3, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t4, &attr, f, (any_t)50);
      *marcel_stats_get(t4, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t5, &attr, f, (any_t)50);
      *marcel_stats_get(t5, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t6, &attr, f, (any_t)50);
      *marcel_stats_get(t6, load) = 10;
   }

   //création de la bulle 4 :
   marcel_bubble_init(&b4);
   marcel_bubble_insertbubble(&b1, &b4);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b4);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t7, &attr, f, (any_t)50);
      *marcel_stats_get(t7, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b4);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t8, &attr, f, (any_t)50);
      *marcel_stats_get(t8, load) = 10;
   }

   //création de la bulle 5 :
   marcel_bubble_init(&b5);
   marcel_bubble_insertbubble(&b1, &b5);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b5);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t9, &attr, f, (any_t)50);
      *marcel_stats_get(t9, load) = 10;
   }

   //création de la bulle 6 :
   marcel_bubble_init(&b6);
   marcel_bubble_insertbubble(&b1, &b6);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b6);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t10, &attr, f, (any_t)50);
      *marcel_stats_get(t10, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b6);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t11, &attr, f, (any_t)50);
      *marcel_stats_get(t11, load) = 10;
   }

   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b6);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t12, &attr, f, (any_t)50);
      *marcel_stats_get(t12, load) = 10;
   }

   //création de la bulle 7 :
   marcel_bubble_init(&b7);
   marcel_bubble_insertbubble(&b1, &b7);
   //création du thread id = 0 :
   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b7);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr, MA_DEF_PRIO);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t13, &attr, f, (any_t)50);
      *marcel_stats_get(t13, load) = 10;
   }

   marcel_start_playing();
   fprintf(stderr, "wake up de la bulle %p\n", &b0);
   marcel_wake_up_bubble(&b0);

   marcel_printf("ok\n");
   marcel_end();

   profile_stop();
   return 0;
}
#else /* MA__BUBBLES */
int main(int argc, char *argv[]) {
   fprintf(stderr,"%s needs bubbles\n",argv[0]);
   return 0;
}
#endif /* MA__BUBBLES */
