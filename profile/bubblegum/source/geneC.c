#include "geneC.h"

FILE * fw;
static int bid;
static int tid;

/* Fonction de parcours de bulle
   si mode est placé à GENERE, écriture dans le fichier généré
   sinon VISITE our simple parcours */

int parcourir_bulle(Element* bulle, int mybid, int mode) 
{
   int i;
   Element * element_i;
   TypeElement type;

   if (mybid == 0)
   {
	  if (mode == 1)
      {
         fwprintf(fw,L"   //création de la bulle %d :\n",mybid);
	 fprintf(fw,"   marcel_bubble_init(&b%d);\n",mybid);
	 //fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",mybid,GetPrioriteBulle(bulle));
      }
	  if (mode == 2)
      {
         fprintf(fw,"marcel_bubble_t b0;\n");
      }
   }
   int taillebulle = GetNbElement(bulle);
   for (i = 1; i <= taillebulle; i++)
   {
	  element_i = GetElement(bulle, i);
	  type = GetTypeElement(element_i);
	  if (type == BULLE)
      {
   	int hisbid = ++bid;
         if(mode == 1)
         {
            fwprintf(fw,L"   //création de la bulle %d :\n",hisbid);
            fprintf(fw,"   marcel_bubble_init(&b%d);\n",hisbid);
            fprintf(fw,"   marcel_bubble_insertbubble(&b%d, &b%d);\n",mybid,hisbid);
            //fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",hisbid,GetPrioriteBulle(element_i));
         }
         if(mode == 2)
         {
            fprintf(fw,"marcel_bubble_t b%d;\n",hisbid);
         }
         parcourir_bulle(element_i,hisbid,mode);
      }  
	  if (type == THREAD)
      {
         if(mode == 1)
         {
            fwprintf(fw,L"   //création du thread id = %d :\n",GetId(element_i));
            fprintf(fw,"   {\n");
            fprintf(fw,"      marcel_attr_t attr;\n");
            fprintf(fw,"      marcel_attr_init(&attr);\n");
            fprintf(fw,"      marcel_attr_setinitbubble(&attr, &b%d);\n",mybid);
            fprintf(fw,"      marcel_attr_setid(&attr,%d);\n",GetId(element_i));
            fprintf(fw,"      marcel_attr_setprio(&attr,%d);\n",GetPrioriteThread(element_i));		  
            fprintf(fw,"      marcel_attr_setname(&attr,\"%s\");\n",GetNom(element_i));
            fprintf(fw,"      marcel_create(&t%d, &attr, f, (any_t)(intptr_t)%d);\n",tid,GetId(element_i)*100+GetCharge(element_i));
	    fprintf(fw,"      *marcel_stats_get(t%d, marcel_stats_load_offset) = %d;\n",tid,GetCharge(element_i));
            fprintf(fw,"   }\n\n");
         }
         if(mode == 2)
         {
            fprintf(fw,"marcel_t t%d;\n",tid); 
         }
         tid ++;
      }
   }
   return 0;
}

/* Fonction principale de génération du fichier .C */

int gen_fichier_C(Element * bullemere)
{
   if (GetTypeElement(bullemere) != BULLE)
   {
	  wprintf(L"Le fichier entré en paramètre n'est pas une BULLE\n"); 
	  return -1;
   }
   bid = 0;
   tid = 1;
   fw = fopen(GENEC_NAME ".c","w");
   if (fw == NULL)
   {
	  wprintf(L"Erreur lors de l'ouverture du fichier en écriture\n"); 
	  return -1;
   }

   wprintf(L"**** Démarrage de la génération du fichier .C ****\n");

   fprintf(fw,"#include \"marcel.h\"\n\n");
   fprintf(fw,"any_t f(any_t foo) {\n");
   fprintf(fw,"   int i = (intptr_t)foo;\n");
   fprintf(fw,"   int id = i/100;\n");
   fprintf(fw,"   int load = i%%100;\n");
   fprintf(fw,"   int n;\n");
   fprintf(fw,"   int sum = 0;\n");
   fprintf(fw,"   marcel_printf(\"some work %%d in %%d\\n\",load,id);\n");
   fprintf(fw,"   if (!load) marcel_delay(1000);\n");
   fprintf(fw,"   for (n=0;n<load*10000000;n++) sum+=n;\n");
   fprintf(fw,"   marcel_printf(\"%%d done\\n\",id);\n");
   fprintf(fw,"   return (void*)sum;\n}\n\n");

   parcourir_bulle(bullemere,0,2);
  
   fprintf(fw,"\nint main(int argc, char *argv[]) {");
   fprintf(fw,"\n\n   marcel_init(&argc,argv);");
   fprintf(fw,"\n#ifdef PROFILE");
   fprintf(fw,"\n   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);");
   fprintf(fw,"\n#endif");
   fprintf(fw,"\n   marcel_printf(\"started\\n\");\n\n");
  
   bid = 0;
   tid = 1;
  
   parcourir_bulle(bullemere,0,1);
  
   fprintf(fw,"   marcel_wake_up_bubble(&b0);\n");
   fprintf(fw,"   marcel_start_playing();\n");
   fprintf(fw,"   while(1) {\n");
   fprintf(fw,"   marcel_bubble_spread(&b0, marcel_topo_level(0,0));\n");
   fprintf(fw,"   marcel_delay(100); }\n");
  
   fprintf(fw,"\n   marcel_printf(\"ok\\n\");\n");
   //fprintf(fw,"\n   profile_stop();\n");
   fprintf(fw,"\n   marcel_end();\n");
   fprintf(fw,"   return 0;\n}\n");

   fclose(fw);
  
   wprintf(L"**** Fichier .C généré :) ****\n\n");


   fw = fopen(GENEC_MAKEFILE,"w");
   if (fw == NULL)
   {
	  wprintf(L"Erreur lors de l'ouverture du fichier en écriture\n"); 
	  return -1;
   }

   wprintf(L"**** Démarrage de la génération du fichier Makefile ****\n");

   fprintf(fw,"CC\t=\t$(shell pm2-config --cc)\n");
   fprintf(fw,"CFLAGS\t=\t$(shell pm2-config --cflags)\n");
   fprintf(fw,"LIBS\t=\t$(shell pm2-config --libs)\n\n");

   fprintf(fw,"all: %s\n", GENEC_NAME);
   fprintf(fw,"%s: %s.o\n", GENEC_NAME, GENEC_NAME);
   fprintf(fw,"\t$(CC) $^ $(LIBS) -o $@\n");
   fprintf(fw,"%s.o: %s.c\n", GENEC_NAME, GENEC_NAME);
   fprintf(fw,"\t$(CC) $(CFLAGS) -c $< -o $@\n");

   fclose(fw);

   return 0;
}
