
%{
  /*#define YYSTYPE long */
#define YYDEBUG 1
#define LEOPARSE_IN_YACC
#define YYERROR_VERBOSE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "leoparse.h"

  /* Lex interface */
  extern FILE *yyin, *yyout;
  extern int yylex(void);

  /* Global variables */
  p_tbx_htable_t leo_parser_result = NULL;

  /* Prototypes */
  void yyerror(char *s); 
%}

%union
{
  char                      *str;
  char                      *id;
  p_tbx_slist_t              list;
  p_tbx_htable_t             htable;
  p_leoparse_htable_entry_t  htable_entry;
  p_leoparse_object_t        object;
}

%token <str> LEOP_ID LEOP_STRING 
%token '{' '}' ':' ';' ',' INCONNU
%type <object>             leop_object
%type <list>               leop_list
%type <htable_entry>       leop_htable_entry
%type <htable>             leop_htable
%type <htable>             leop_file
%type <htable>             entree
%%

entree:  
  leop_file
{
  $$ = $1;
}
;

leop_file:
  leop_htable ';'
{
  $$ = $1;
}
;

leop_htable:
  leop_htable ';' leop_htable_entry
{
  tbx_htable_add($1, $3->id, $3->slist);
  free($3->id);
  $3->id    = NULL;
  $3->slist = NULL;
  free($3);
  $3 = NULL;  
}
| leop_htable_entry
{
  $$ = malloc(sizeof(tbx_htable_t));
  tbx_htable_init($$, 0);
  tbx_htable_add($$, $1->id, $1->slist);
  free($1->id);
  $1->id    = NULL;
  $1->slist = NULL;
  free($1);
  $1 = NULL;
}
;

leop_htable_entry:
  LEOP_ID ':' leop_list
{
  $$ = malloc(sizeof(leoparse_htable_entry_t));
  $$->id    = $1;
  $$->slist = $3;
}
;

leop_list:
  leop_list ',' leop_object
{
  tbx_slist_enqueue($1, $3);  
}
| leop_object
{
  $$ = tbx_slist_nil();
  tbx_slist_enqueue($$, $1);
}
;

leop_object:
  '{' leop_htable '}'
{
  $$ = calloc(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_htable;
  $$->htable = $2;
}
| LEOP_ID
{
  $$ = calloc(1, sizeof(leoparse_object_t));
  $$->type = leoparse_id;
  $$->id   = $1;
}
| LEOP_STRING
{
  $$ = calloc(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_string;
  $$->string = $1;
}
;

%%

#include <stdlib.h>
#include <stdio.h>
#define DEBUG_YACC

void yyerror(char *s)
{
  fflush(stdout);
  fprintf(stderr, "%s\n", s);
}

