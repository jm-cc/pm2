/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*
 * Leonie.c
 * ========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <leonie.h>

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern FILE *yyin;
extern FILE *yyout;
extern p_leo_parser_result_t leo_parser_result;

int 
yylex(void);


/*
 * Functions
 * ---------
 */
int main (int argc, char *argv[])
{
  FILE *cluster_description;
  FILE *application_description;
  FILE *protocol_description;
  
  if (argc != 4) 
    {
      printf("usage: leonie <filename1> <filename2> <filename3>\n");
      printf("  filename1 == cluster description file name\n");
      printf("  filename2 == application description file name\n");
      printf("  filename3 == protocol description file name\n");
      exit(0);
    }

  tbx_init();
  ntbx_init();
  
  cluster_description     = fopen(argv[1], "r");
  application_description = fopen(argv[2], "r");
  protocol_description = fopen(argv[3], "r");

  printf("Parsing cluster description file ... ");
  yyin = cluster_description;
  if (yyparse() == 0)
    {
      printf("Parsing succeeded\n");
    }
  else
    {
      printf("Parsing failed\n");
    }

  printf("Parsing application description file ... ");
  yyin = application_description;
  if (yyparse() == 0)
    {
      printf("Parsing succeeded\n");
    }
  else
    {
      printf("Parsing failed\n");
    }
 
  printf("Parsing protocol description file ... ");
  yyin = protocol_description;
  if (yyparse() == 0)
    {
      printf("Parsing succeeded\n");
    }
  else
    {
      printf("Parsing failed\n");
    }
  
  return 0;
}
