#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* s'envoie un signal raise trois fois */
volatile int fini1,fini2;

static void traitant2(int signo)
{
   fini2 ++;
   fprintf(stderr,"--- %d fois t1 dans traitant2 ---\n",fini2);
   fprintf(stderr,"c'est le signal numero %d\n",signo);
   if (fini2<3)
      raise(SIGUSR1);
   fprintf(stderr,"--- %d fois sorti du traitant2 ---\n",fini2);
   return;
}
static void traitant1(int signo)
{
   fini1 ++;
   fprintf(stderr,"--- %d fois t1 dans traitant1 ---\n",fini1);
   fprintf(stderr,"c'est le signal numero %d\n",signo);
   if (fini1<3)
      raise(SIGUSR2);
   fprintf(stderr,"--- %d fois sorti du traitant1 ---\n",fini1);
   return;
}

void *recsig(void * arg)
{   
   fini1 = 0;
   fini2 = 0;
   fprintf(stderr,"entree de t1 dans recsig -> raise(%d)\n",SIGUSR1);
   raise(SIGUSR1);
   raise(SIGUSR2);
   fprintf(stderr,"sortie de t1 de recsig : pthread_exit(1)\n");   
   pthread_exit((void *)1);
}

int main(int argc, char *argv[])
{
   fprintf(stderr,"entree dans le main\n");
   void * value_ptr1;
   
   struct sigaction act;
   act.sa_handler = traitant1;
   fprintf(stderr,"adresse traitant : %p\n",traitant1);
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags = SA_RESTART|SA_NODEFER;
   if (!sigaction(SIGUSR1,&act,NULL))
      fprintf(stderr,"sigaction ok : traitant1 associe a %d\n",SIGUSR1);
   else
      fprintf(stderr,"erreur sigaction\n");
   
   act.sa_handler = traitant2;
   if (!sigaction(SIGUSR2,&act,NULL))
      fprintf(stderr,"sigaction ok : traitant2 associe a %d\n",SIGUSR2);
   else
      fprintf(stderr,"erreur sigaction\n");

   pthread_t t1;
   pthread_attr_t attr;

   fprintf(stderr,"structure attr inititialisee\n");
   pthread_attr_init(&attr);
      
   fprintf(stderr,"t1 est cree avec attr : %p\nil doit executer recsig\n",&t1);
   pthread_create(&t1,&attr,recsig,NULL);

   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
 
   return 0;
}

