#include "geneC.h"
#include "interfaceGauche.h"

FILE * fw;

/* Fonction de parcours de bulle
   mode DECLARE ne fait que déclarer les variables de manière globale,
   mode GENERE produit le code */

int parcourir_bulle(Element* bulle, int mybid) 
{
  int id;
  Element * element_i;
  ListeElement *liste;
  TypeElement type;

  for (liste = FirstListeElement(bulle); liste; liste = NextListeElement(liste))
    {
      element_i = liste->element;
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
	  else
	    fprintf(fw,"   marcel_bubble_insertbubble(&marcel_root_bubble, &b%d);\n",id);
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
	  fprintf(fw,"      marcel_create(&t%d, &attr, f, (any_t)(intptr_t)%d);\n",id,id*MAX_CHARGE+GetCharge(element_i));
	  fprintf(fw,"      *marcel_stats_get(t%d, load) = %d;\n",id,GetCharge(element_i));
	  fprintf(fw,"   }\n\n");
	}
    }
  return 0;
}

static void add_headers (FILE *fd)
{
  fprintf (fd, "#include <marcel.h>\n\n");  
}

static void add_static_functions (FILE *fd)
{
  /* Write down the _f_ function. */
  fprintf (fd, "any_t f (any_t foo) {\n");
  fprintf (fd, "   int i = (intptr_t)foo;\n");
  fprintf (fd, "   int id = i / %d;\n", MAX_CHARGE);
  fprintf (fd, "   int load = i %% %d;\n", MAX_CHARGE);
  fprintf (fd, "   int n;\n");
  fprintf (fd, "   int sum = 0;\n\n");
  fprintf (fd, "   marcel_printf (\"some work %%d in %%d\\n\", load, id);\n\n");
  fprintf (fd, "   if (!load)\n"); 
  fprintf (fd, "     marcel_delay (1000);\n\n");
  fprintf (fd, "   for (n = 0; n < load * 10000000; n++)\n");
  fprintf (fd, "     sum += n;\n\n");
  fprintf (fd, "   marcel_printf (\"%%d done\\n\", id);\n\n");
  fprintf (fd, "   return (void*)(intptr_t)sum;\n}\n\n");  
}

static void add_main_function (FILE *fd, Element *root_bubble)
{
  fprintf (fw, "int main (int argc, char *argv[]) {\n");
  fprintf (fw, "   marcel_init (&argc, argv);\n");
  fprintf (fw, "#ifdef PROFILE\n");
  fprintf (fw, "   profile_activate (FUT_ENABLE, MARCEL_PROF_MASK, 0);\n");
  fprintf (fw, "#endif\n\n");
  fprintf (fw, "   marcel_printf (\"started\\n\");\n\n");
  
  parcourir_bulle (root_bubble, 0);
  
  fprintf (fw, "   marcel_start_playing ();\n");
  fprintf (fw, "   marcel_bubble_sched_begin ();\n");
  fprintf (fw, "   marcel_delay (1000);\n"); 
  fprintf (fw, "   marcel_printf (\"ok\\n\");\n");
  fprintf (fw, "   marcel_end ();\n\n");
  fprintf (fw, "   return 0;\n}\n");
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
   
  add_headers (fw);
  add_static_functions (fw);
  add_main_function (fw, bullemere);

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
  fprintf(fw,"modules\t\t:=\t$(shell pm2-config --modules)\n");
  fprintf(fw,"stamplibs\t:=\t$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)\n\n");

  fprintf(fw,"all: %s\n", GENEC_NAME);
  fprintf(fw,"%s: %s.o ${stamplibs}\n", GENEC_NAME, GENEC_NAME);
  fprintf(fw,"\t$(CC) $< $(LIBS) -o $@\n");
  fprintf(fw,"%s.o: %s.c\n", GENEC_NAME, GENEC_NAME);
  fprintf(fw,"\t$(CC) $(CFLAGS) -c $< -o $@\n");

  fclose(fw);

  return 0;
}
