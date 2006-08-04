#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* le raise n'est apellé qu'une fois */

volatile int fini;

static void traitant(int signo)
{
   fini ++;
   fprintf(stderr,"--- %d fois t1 dans traitant : signal %d ---\n",fini,signo);
   if (fini<1000)
   {
      fprintf(stderr,"- raise(%d) -\n",signo);
      raise(SIGUSR2);
   }
   fprintf(stderr,"%d\n",fini);   
   fprintf(stderr,"--- t1 sorti du traitant ---\n");
   return;
}



int main(int argc, char *argv[])
{
   fprintf(stderr,"thread main : %lx\n",pthread_self());

   struct sigaction act;
   act.sa_handler = traitant;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags = SA_RESTART;
   sigaction(SIGUSR2,&act,NULL);
   
	raise(SIGUSR2);

   usleep(1000000);

   return 0;
}

