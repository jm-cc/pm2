#include "geneC.h"

FILE * fw;

/* Fonction de parcours de bulle
   mode DECLARE ne fait que déclarer les variables de manière globale,
   mode GENERE produit le code */

int parcourir_bulle(Element* bulle, int mybid) 
{
   int i, id;
   Element * element_i;
   TypeElement type;

   int taillebulle = GetNbElement(bulle);
   for (i = 1; i <= taillebulle; i++)
   {
	  element_i = GetElement(bulle, i);
	  type = GetTypeElement(element_i);
	  id = GetId(element_i);
	  if (type == BULLE)
      {
            fprintf(fw,"marcel_bubble_t b%d;\n",id);
            fwprintf(fw,L"   //création de la bulle %d :\n",id);
            fprintf(fw,"   marcel_bubble_init(&b%d);\n",id);
	    fprintf(fw,"   marcel_bubble_setid(&b%d, %d);\n",id,id);
	    if (mybid>0)
                fprintf(fw,"   marcel_bubble_insertbubble(&b%d, &b%d);\n",mybid,id);
            //fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",id,GetPrioriteBulle(element_i));
            parcourir_bulle(element_i,id);
      }  
	  if (type == THREAD)
      {
            fprintf(fw,"marcel_t t%d;\n",id); 
            fwprintf(fw,L"   //création du thread id = %d :\n",id);
            fprintf(fw,"   {\n");
            fprintf(fw,"      marcel_attr_t attr;\n");
            fprintf(fw,"      marcel_attr_init(&attr);\n");
	    if (mybid)
               fprintf(fw,"      marcel_attr_setinitbubble(&attr, &b%d);\n",mybid);
            fprintf(fw,"      marcel_attr_setid(&attr,%d);\n",id);
            //fprintf(fw,"      marcel_attr_setprio(&attr,%d);\n",GetPrioriteThread(element_i));		  
            fprintf(fw,"      marcel_attr_setname(&attr,\"%s\");\n",GetNom(element_i));
            fprintf(fw,"      marcel_create(&t%d, &attr, f, (any_t)(intptr_t)%d);\n",id,id*100+GetCharge(element_i));
	    fprintf(fw,"      *marcel_stats_get(t%d, marcel_stats_load_offset) = %d;\n",id,GetCharge(element_i));
            fprintf(fw,"   }\n\n");
      }
   }
   return 0;
}

/* Fonction principale de génération du fichier .C */

int gen_fichier_C(const char * fichier, Element * bullemere)
{
   if (GetTypeElement(bullemere) != BULLE)
   {
	  wprintf(L"Le fichier entré en paramètre n'est pas une BULLE\n"); 
	  return -1;
   }
   fw = fopen(fichier, "w");
   if (fw == NULL)
   {
	  wprintf(L"Erreur lors de l'ouverture du fichier en écriture\n"); 
	  return -1;
   }

   wprintf(L"**** Début de la génération du fichier '%s' ****\n", fichier);

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
   fprintf(fw,"\nint main(int argc, char *argv[]) {");
   fprintf(fw,"\n\n   marcel_init(&argc,argv);");
   fprintf(fw,"\n#ifdef PROFILE");
   fprintf(fw,"\n   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);");
   fprintf(fw,"\n#endif");
   fprintf(fw,"\n   marcel_printf(\"started\\n\");\n\n");
  
   parcourir_bulle(bullemere,0);
  
   int taillebulle = GetNbElement(bullemere);
   int i;
   for (i = 1; i <= taillebulle; i++) {
      Element *element_i = GetElement(bullemere, i);
      TypeElement type = GetTypeElement(element_i);
      int id = GetId(element_i);
      if (type == BULLE)
         fprintf(fw,"      marcel_wake_up_bubble(&b%d);\n", id);
   }

   fprintf(fw,"   marcel_start_playing();\n");
   fprintf(fw,"   int i = 5;\n");
   fprintf(fw,"   while(i--) {\n");

   for (i = 1; i <= taillebulle; i++) {
      Element *element_i = GetElement(bullemere, i);
      TypeElement type = GetTypeElement(element_i);
      int id = GetId(element_i);
      if (type == BULLE)
         fprintf(fw,"      marcel_bubble_spread(&b%d, marcel_topo_level(0,0));\n", id);
   }
   fprintf(fw,"      marcel_delay(1000);\n");
   fprintf(fw,"   }\n");
  
   fprintf(fw,"\n   marcel_printf(\"ok\\n\");\n");
   //fprintf(fw,"\n   profile_stop();\n");
   fprintf(fw,"\n   marcel_end();\n");
   fprintf(fw,"   return 0;\n}\n");

   fclose(fw);
  
   wprintf(L"**** Fichier '%s' généré ****\n\n", fichier);


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
