/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <assert.h>

#include "geneC.h"
#include "interfaceGauche.h"

/* Browse bubble _from_bubble_ and generates the code involved in the
   creation of the corresponding threads and bubbles tree. */
static void
generate_bubble_tree (FILE *fd, Element* from_bubble, int mybid)
{
  int id;
  Element * element_i;
  ListeElement *liste;
  TypeElement type;
  char *parent_bubble_id;

  asprintf (&parent_bubble_id, "%i", mybid);

  for (liste = FirstListeElement (from_bubble); liste; liste = NextListeElement (liste))
    {
      element_i = liste->element;
      type = GetTypeElement (element_i);
      id = GetId (element_i);

      if (type == BULLE)
	{
	  fprintf (fd, "   /* Creating bubble %d */\n", id);
	  fprintf (fd, "   marcel_bubble_t b%d;\n", id);
	  fprintf (fd, "   bubblegum_create_bubble (&b%d, %d, %s%s);\n",
		   id,
		   id,
		   mybid == 0 ? "&marcel_root_bubble" : "&b",
		   mybid == 0 ? "" : parent_bubble_id);
	  generate_bubble_tree (fd, element_i, id);
	}
      if (type == THREAD)
	{
	  fprintf (fd, "   marcel_t t%d;\n", id);
	  fprintf (fd, "   /* Creating thread %d */\n", id);
	  fprintf (fd, "   bubblegum_create_thread (&t%d, %d, \"%s\", %s, %d, %s%s);\n",
		   id,
		   id,
		   GetNom (element_i),
		   "f",
		   GetCharge (element_i),
		   mybid == 0 ? "&marcel_root_bubble" : "&b",
		   mybid == 0 ? "" : parent_bubble_id);
	}
    }

  free (parent_bubble_id);
}

static void
add_headers (FILE *fd)
{
  fprintf (fd, "#include <marcel.h>\n\n");
}

static void
add_static_functions (FILE *fd)
{
  /* Write down the bubblegum_create_bubble () function */
  fprintf (fd, "static void bubblegum_create_bubble (marcel_bubble_t *bubble, int id, marcel_bubble_t *parent_bubble) {\n");
  fprintf (fd, "  marcel_bubble_init (bubble);\n");
  fprintf (fd, "  marcel_bubble_setid (bubble, id);\n\n");
  fprintf (fd, "  marcel_bubble_insertbubble ((parent_bubble != NULL) ? parent_bubble : &marcel_root_bubble, bubble);\n}\n\n");

  /* Write down the bubblegum_create_thread () function */
  fprintf (fd, "static void bubblegum_create_thread (marcel_t *thread,\n");
  fprintf (fd, "                                     int id,\n");
  fprintf (fd, "                                     char *name,\n");
  fprintf (fd, "                                     void *(*start_routine)(void *),\n");
  fprintf (fd, "                                     long wload,\n");
  fprintf (fd, "                                     marcel_bubble_t *parent_bubble) {\n");
  fprintf (fd, "  marcel_attr_t thread_attr;\n");
  fprintf (fd, "  marcel_attr_init (&thread_attr);\n");
  fprintf (fd, "  marcel_attr_setinitbubble (&thread_attr, parent_bubble);\n");
  fprintf (fd, "  marcel_attr_setid (&thread_attr, id);\n");
  fprintf (fd, "  marcel_attr_setname (&thread_attr, name);\n\n");
  fprintf (fd, "  marcel_create (thread, &thread_attr, start_routine, (any_t)(intptr_t)wload);\n");
  fprintf (fd, "  *marcel_stats_get (*thread, load) = wload;\n}\n\n");

  /* Write down the f () function. */
  fprintf (fd, "static void *f (void *foo) {\n");
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

static void
add_main_function (FILE *fd, Element *root_bubble)
{
  fprintf (fd, "int main (int argc, char *argv[]) {\n");
  fprintf (fd, "   marcel_init (argc, argv);\n");
  fprintf (fd, "#ifdef PROFILE\n");
  fprintf (fd, "   profile_activate (FUT_ENABLE, MARCEL_PROF_MASK, 0);\n");
  fprintf (fd, "#endif\n\n");
  fprintf (fd, "   marcel_printf (\"started\\n\");\n\n");

  generate_bubble_tree (fd, root_bubble, 0);

  fprintf (fd, "   marcel_start_playing ();\n");
  fprintf (fd, "   marcel_bubble_sched_begin ();\n");
  fprintf (fd, "   marcel_delay (1000);\n");
  fprintf (fd, "   marcel_printf (\"ok\\n\");\n");
  fprintf (fd, "   marcel_end ();\n\n");
  fprintf (fd, "   return 0;\n}\n");
}

static void
generate_makefile (FILE *fd)
{
  printf ("**** Generating Makefile ****\n");

  fprintf (fd, "CC\t=\t$(shell pm2-config --cc)\n");
  fprintf (fd, "CFLAGS\t=\t$(shell pm2-config --cflags)\n");
  fprintf (fd, "LIBS\t=\t$(shell pm2-config --libs)\n\n");
  fprintf (fd, "modules\t\t:=\t$(shell pm2-config --modules)\n");
  fprintf (fd, "stamplibs\t:=\t$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)\n\n");

  fprintf (fd, "all: %s\n", GENEC_NAME);
  fprintf (fd, "%s: %s.o ${stamplibs}\n", GENEC_NAME, GENEC_NAME);
  fprintf (fd, "\t$(CC) $< $(LIBS) -o $@\n");
  fprintf (fd, "%s.o: %s.c\n", GENEC_NAME, GENEC_NAME);
  fprintf (fd, "\t$(CC) $(CFLAGS) -c $< -o $@\n");

  printf ("**** Makefile successfully generated ****\n");
}

/* Main function for generating the C example program. */
int
gen_fichier_C (const char * file, Element * root_bubble)
{
  FILE *fw;

  assert (GetTypeElement (root_bubble) == BULLE);

  fw = fopen (file, "w");
  if (fw == NULL)
    {
      printf ("Error while loading file '%s'.\n", file);
      return -1;
    }

  printf ("**** Generating the '%s' file ****\n", file);

  add_headers (fw);
  add_static_functions (fw);
  add_main_function (fw, root_bubble);

  fclose(fw);

  printf ("**** File '%s' successfully generated ****\n\n", file);

  fw = fopen (GENEC_MAKEFILE, "w");
  if (fw == NULL)
    {
      printf ("Error while loading file '%s'.\n", GENEC_MAKEFILE);
      return -1;
    }

  generate_makefile (fw);

  fclose (fw);

  return 0;
}
