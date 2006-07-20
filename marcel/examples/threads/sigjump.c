#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

sigjmp_buf contexte;

void gestionnaire_sigfpe(int numero)
{
   fprintf(stderr,"on est dans le traitant\n");
   fprintf(stderr,"on va faire le longjmp\n");
   siglongjmp(contexte,1);
   fprintf(stderr,"longjump rate\n");
}

int main(int argc, char *argv[])
{
   int p,q,r;

   struct sigaction action;
   action.sa_handler = gestionnaire_sigfpe;
   action.sa_flags = 0;
   sigfillset(&action.sa_mask);
   sigaction(SIGFPE,&action,NULL);

   if (sigsetjmp(contexte,1) != 0) {
      fprintf(stderr,"on est arrive ici par siglongjmp\n");
      fprintf(stderr,"!! erreur de mathematique\n");
   }
   while(1)
   {
      fprintf(stderr,"entrer le dividende p :\n");
      if (fscanf(stdin,"%d",&p) == 1)
         break;
   }
   while(1)
   {
      fprintf(stderr,"entrer le dividende p :\n");
      if (fscanf(stdin,"%d",&q) == 1)
         break;
   }
   r = p/q;
   fprintf(stderr,"resultat p/q : %d\n",r);
    
   return 0;
}

