#include "geneC.h"

#define VISITE 0
#define GENERE 1

FILE * fw;
static int bid = 0;
static int tid = 0;

/* Fonction de parcours de bulle
   si mode est plac� � GENERE, �criture dans le fichier g�n�r�
   sinon VISITE our simple parcours */

int parcourir_bulle(Element* bulle, int where, int mode)
{
  int i;
  int faux_bid;
  Element * element_i;
  TypeElement type;

  if (where == 0)
	{
	  if (mode == 1)
		{
		  fprintf(fw,"   //cr�ation de la bulle %d :\n",bid);
		  fprintf(fw,"   marcel_bubble_init(&b%d);\n",bid);
		  fprintf(fw,"   marcel_bubble_setprio(&b%d,%d);\n\n",bid,GetPrioriteBulle(bulle));
		}
	  if (mode == 2)
		{
		  fprintf(fw,"marcel_bubble_t b0;\n");
		}
	  faux_bid = 0;
	}

  for (i = 0; i < GetNbElement(bulle); i++)
	{
	  element_i = GetElement(bulle,i);
	  type = GetTypeElement(element_i);
	  if (type == BULLE)
		{
		  bid ++;
		  if(mode == 1)
			{
			  fprintf(fw,"   //cr�ation de la bulle %d :\n",bid);
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
			  fprintf(fw,"   //cr�ation du thread id = %d :\n",GetId(element_i));
			  fprintf(fw,"   {\n");
			  fprintf(fw,"      marcel_attr_t attr;\n");
			  fprintf(fw,"      marcel_attr_init(&attr);\n");
			  fprintf(fw,"      marcel_attr_setinitbubble(&attr, &b%d);\n",where);
			  fprintf(fw,"      marcel_attr_setid(&attr,%d);\n",GetId(element_i));
			  fprintf(fw,"      marcel_attr_setprio(&attr,%d);\n",GetPrioriteThread(element_i));		  
			  fprintf(fw,"      marcel_attr_setname(&attr,\"%s\");\n",GetNom(element_i));
			  fprintf(fw,"      marcel_create(&t%d, &attr, f, (any_t)%d);\n",GetId(element_i),GetId(element_i));
			  fprintf(fw,"   }\n\n");
			}
		  if(mode == 2)
			{
			  fprintf(fw,"marcel_t t%d;\n",GetId(element_i)); 
			}
		  tid ++;
		}
	}
  for(i = 0; i < GetNbElement(bulle); i++)
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

/* Fonction principale de g�n�ration du fichier .C */

int gen_fichier_C(Element * bullemere)
{
  if (GetTypeElement(bullemere) != BULLE)
	{
	  printf("Le fichier entr� en param�tre n'est pas une BULLE\n"); 
	  return -1;
	}
  fw = fopen("to_pm2.c","w");
  if (fw == NULL)
	{
	  printf("Erreur lors de l'ouverture du fichier en �criture\n"); 
	  return -1;
	}

  printf("**** D�marrage de la g�n�ration du fichier .C ****\n");

  fprintf(fw,"#include \"marcel.h\"\n\n");
  fprintf(fw,"any_t f(any_t foo) {\n");
  fprintf(fw,"   int i = (int)foo;\n");
  fprintf(fw,"   marcel_printf(\"some work in %%d\\n\",i);\n");
  fprintf(fw,"   return NULL;\n}\n\n");

  //parcourir_bulle(bullemere,0,VISITE);
  parcourir_bulle(bullemere,0,2);
  //generer_decl(tid,bid);
  
  fprintf(fw,"\nint main(int argc, char *argv[]) {");
  fprintf(fw,"\n\n   marcel_init(&argc,argv);");
  fprintf(fw,"\n   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);");
  fprintf(fw,"\n   marcel_printf(\"started\\n\");\n\n");
  
  bid = 0;
  tid = 1;
  
  parcourir_bulle(bullemere,0,GENERE);
  
  fprintf(fw,"   marcel_wake_up_bubble(&b0);\n");
  
  fprintf(fw,"\n   marcel_printf(\"ok\\n\");");
  fprintf(fw,"\n   profile_stop();\n");
  fprintf(fw,"   marcel_end();\n\n");
  fprintf(fw,"   return 0;\n}\n");

  fclose(fw);
  
  printf("**** Fichier .C g�n�r� :) ****\n\n");

  return 0;
}
