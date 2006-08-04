#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* s'envoie un signal raise 10000 fois en nodefer */

/* segfault au bout de plusieurs centaines de fois */
/* si on fait un pthread_kill(pthread_self) à la place
   cela segfault au bout de : - 16 fois avec le defer de deliver_sig
                            	- 11 fois sans le defer de deliver_sig */
/* contrairement au sigtrombe-test3 le main ne vient pas demander le raise */

volatile int fini;

static void traitant(int signo)
{
   fini ++;
   fprintf(stderr,"--- %d fois t1 dans traitant : signal %d ---\n",fini,signo);
   if (fini<10000)
   {
      fprintf(stderr,"- raise(%d) -\n",signo);
      raise(SIGUSR2);
   }
   fprintf(stderr,"%d\n",fini);
   fprintf(stderr,"--- t1 sorti du traitant ---\n");
   return;
}

void *recsig(void * arg)
{   
   fini = 0;
   fprintf(stderr,"entree de t1 dans recsig -> raise(%d)\n",SIGUSR2);
   fprintf(stderr,"- raise(%d) -\n",SIGUSR2);
   raise(SIGUSR2);
   fprintf(stderr,"sortie de t1 de recsig : pthread_exit(1)\n");   
   pthread_exit((void *)1);
}

int main(int argc, char *argv[])
{
   pthread_t t1;
   void * value_ptr1;
   
   struct sigaction act;
   act.sa_handler = traitant;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags = SA_RESTART|SA_NODEFER;
   sigaction(SIGUSR2,&act,NULL);
   
   pthread_attr_t attr;

   fprintf(stderr,"structure attr inititialisee\n");
   pthread_attr_init(&attr);
      
   pthread_create(&t1,&attr,recsig,NULL);
   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
 
   return 0;
}

