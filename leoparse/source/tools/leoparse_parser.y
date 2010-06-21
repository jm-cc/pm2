
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
  extern const char *parser_filename;
  extern int yylex(void);

  /* Global variables */
  p_tbx_htable_t leoparse_result = NULL;

  /* Prototypes */
  void yyerror(char *s);
%}

%error-verbose

%union
{
  char                      *str;
  char                      *id;
  int                        val;
  p_leoparse_range_t         range;
  p_leoparse_modifier_t      modifier;
  p_tbx_slist_t              list;
  p_tbx_htable_t             htable;
  p_leoparse_object_t        object;
}

%token <val> LEOP_INTEGER
%token <id> LEOP_ID
%token <str> LEOP_STRING
%token '{' '}' '[' ']' LEOP_RANGE '(' ')' ':' ';' ',' 
%type <range>              leop_range
%type <modifier>           leop_modifier
%type <object>             leop_object
%type <list>               leop_list
%type <htable>             leop_htable
%type <htable>             leop_file
%type <htable>             entree
%%

entree:
  leop_file
{
  $$ = $1;
  leoparse_result = $$;
}
;

leop_file:
  leop_htable
{
  $$ = $1;
}
;

leop_htable:
  leop_htable LEOP_ID ':' leop_object ';'
{
  tbx_htable_add($1, $2, $4);
  $$ = $1;

}
| LEOP_ID ':' leop_object ';'
{
  $$ = TBX_MALLOC(sizeof(tbx_htable_t));
  tbx_htable_init($$, 0);
  tbx_htable_add($$, $1, $3);
}
;

leop_list:
  leop_list ',' leop_object
{
  tbx_slist_append($1, $3);
  $$ = $1;
}
| leop_object
{
  $$ = tbx_slist_nil();
  tbx_slist_append($$, $1);
}
;

leop_object:
  '{' leop_htable '}'
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_o_htable;
  $$->htable = $2;
}
| LEOP_ID
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type = leoparse_o_id;
  $$->id   = $1;
}
| LEOP_ID leop_modifier
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type = leoparse_o_id;
  $$->id   = $1;
  $$->modifier = $2;
}
| LEOP_STRING
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_o_string;
  $$->string = $1;
}
| LEOP_INTEGER
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_o_integer;
  $$->val = $1;
}
| leop_range
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_o_range;
  $$->range = $1;
}
| '(' leop_list ')'
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_object_t));
  $$->type   = leoparse_o_slist;
  $$->slist = $2;
}
;

leop_modifier:
'[' leop_list ']'
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_modifier_t));
  $$->type = leoparse_m_sbracket;
  $$->sbracket = $2;
}
|'(' leop_list ')'
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_modifier_t));
  $$->type = leoparse_m_parenthesis;
  $$->sbracket = $2;
}
;

leop_range:
LEOP_INTEGER LEOP_RANGE LEOP_INTEGER
{
  $$ = TBX_CALLOC(1, sizeof(leoparse_range_t));
  $$->begin = $1;
  $$->end   = $3 + 1;
}
;

%%

#include <stdlib.h>
#include <stdio.h>
#define DEBUG_YACC

extern YYLTYPE yylloc;

void yyerror(char *s)
{
  fflush(stdout);
  fprintf(stderr, "%s at %s:%d:%d\n", s, parser_filename, yylloc.first_line, yylloc.first_column);
}

