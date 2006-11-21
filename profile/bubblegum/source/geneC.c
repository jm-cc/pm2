#include "geneC.h"

FILE * fw;
static int bid;
static int faux_bid;
static int tid;

/* Fonction de parcours de bulle
   si mode est placé à GENERE, écriture dans le fichier généré
   sinon VISITE our simple parcours */

int parcourir_bulle(Element* bulle, int where, int mode) 
{
   int i;
   Element * element_i;
   TypeElement type;

   if (where == 0)
   {
	  if (mode == 1)
      {
         fwprintf(fw,L"   //création de la bulle %d :\n",bid);
	 fprintf(fw,"   marcel_bubble_init(&b%d);\n",bid);
	 //fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",bid,GetPrioriteBulle(bulle));
      }
	  if (mode == 2)
      {
         fprintf(fw,"marcel_bubble_t b0;\n");
      }
	  faux_bid = 0;
   }
   int taillebulle = GetNbElement(bulle);
   for (i = 1; i <= taillebulle; i++)
   {
	  element_i = GetElement(bulle, i);
	  type = GetTypeElement(element_i);
	  if (type == BULLE)
      {
         bid ++;
         if(mode == 1)
         {
            fwprintf(fw,L"   //création de la bulle %d :\n",bid);
            fprintf(fw,"   marcel_bubble_init(&b%d);\n",bid);
            fprintf(fw,"   marcel_bubble_insertbubble(&b%d, &b%d);\n",where,bid);
            fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",bid,GetPrioriteBulle(element_i));
         }
         if(mode == 2)
         {
            fprintf(fw,"marcel_bubble_t b%d;\n",bid);
         }
      }  
	  if (type == THREAD)
      {
         if(mode == 1)
         {
            fwprintf(fw,L"   //création du thread id = %d :\n",GetId(element_i));
            fprintf(fw,"   {\n");
            fprintf(fw,"      marcel_attr_t attr;\n");
            fprintf(fw,"      marcel_attr_init(&attr);\n");
            fprintf(fw,"      marcel_attr_setinitbubble(&attr, &b%d);\n",where);
            fprintf(fw,"      marcel_attr_setid(&attr,%d);\n",GetId(element_i));
            fprintf(fw,"      marcel_attr_setprio(&attr,%d);\n",GetPrioriteThread(element_i));		  
            fprintf(fw,"      marcel_attr_setname(&attr,\"%s\");\n",GetNom(element_i));
            fprintf(fw,"      marcel_create(&t%d, &attr, f, (any_t)%d);\n",GetId(element_i),GetId(element_i)*100+GetCharge(element_i));
            fprintf(fw,"   }\n\n");
         }
         if(mode == 2)
         {
            fprintf(fw,"marcel_t t%d;\n",GetId(element_i)); 
         }
         tid ++;
      }
   }
   for(i = 1; i <= taillebulle; i++)
   {
	  element_i = GetElement(bulle,i);
	  type = GetTypeElement(element_i);
	  if (BULLE == GetTypeElement(element_i))
      {
         faux_bid ++;
         parcourir_bulle(element_i,faux_bid,mode);
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
   faux_bid = 0;
   tid = 0;
   fw = fopen(GENEC_NAME ".c","w");
   if (fw == NULL)
   {
	  wprintf(L"Erreur lors de l'ouverture du fichier en écriture\n"); 
	  return -1;
   }

   wprintf(L"**** Démarrage de la génération du fichier .C ****\n");

   fprintf(fw,"#include \"marcel.h\"\n\n");
   fprintf(fw,"any_t f(any_t foo) {\n");
   fprintf(fw,"   int i = (int)foo;\n");
   fprintf(fw,"   int id = i/100;\n");
   fprintf(fw,"   int load = i%%100;\n");
   fprintf(fw,"   int n;\n");
   fprintf(fw,"   int sum = 0;\n");
   fprintf(fw,"   marcel_printf(\"some work %%d in %%d\\n\",load,id);\n");
   fprintf(fw,"   for (n=0;n<load*10000000;n++) sum+=n;\n");
   fprintf(fw,"   return (void*)sum;\n}\n\n");

   parcourir_bulle(bullemere,0,2);
  
   fprintf(fw,"\nint main(int argc, char *argv[]) {");
   fprintf(fw,"\n\n   marcel_init(&argc,argv);");
   fprintf(fw,"\n   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);");
   fprintf(fw,"\n   marcel_printf(\"started\\n\");\n\n");
  
   bid = 0;
   tid = 1;
  
   parcourir_bulle(bullemere,0,1);
  
   fprintf(fw,"   marcel_wake_up_bubble(&b0);\n");
   fprintf(fw,"   marcel_bubble_spread(&b0, marcel_topo_level(0,0));\n");
  
   fprintf(fw,"\n   marcel_printf(\"ok\\n\");\n");
   //   fprintf(fw,"\n   profile_stop();\n");
   fprintf(fw,"   marcel_end();\n\n");
   fprintf(fw,"   return 0;\n}\n");

   fclose(fw);
  
   wprintf(L"**** Fichier .C généré :) ****\n\n");

   return 0;
}
