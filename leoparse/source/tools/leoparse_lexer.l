%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "leoparse.h"
#undef DEBUG
#define YY_INPUT(buf, result, max_size) \
   {result = leoparse_yy_input(buf, max_size);}

extern YYLTYPE yylloc;
extern int parser_line_num  ;
extern int parser_column_num;
extern const char *parser_filename;
%}
DIGIT          [0-9]
A              [aA]
B              [bB]
C              [cC]
D              [dD]
E              [eE]
F              [fF]
G              [gG]
H              [hH]
I              [iI]
J              [jJ]
K              [kK]
L              [lL]
M              [mM]
N              [nN]
O              [oO]
P              [pP]
Q              [qQ]
R              [rR]
S              [sS]
T              [tT]
U              [uU]
V              [vV]
W              [wW]
X              [xX]
Y              [yY]
Z              [zZ]
STRING         \"[^"]*\"
WD             [_a-zA-Z/][_a-zA-Z0-9/\-]*
ID             "."?{WD}("."{WD})*
INTEGER        {DIGIT}+
%%
{ID}                               {
  yylval.str = malloc(yyleng + 1);
  strcpy(yylval.str, yytext);
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += strlen(yytext);
  yylloc.last_column  = parser_column_num - 1;
  return LEOP_ID;
}
{STRING}                            {
  yylval.str = malloc(yyleng - 1);
  strncpy(yylval.str, yytext + 1, yyleng - 2);
  yylval.str[yyleng - 1] = '\0';
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += strlen(yytext);
  yylloc.last_column  = parser_column_num - 1;
  return LEOP_STRING;
}
{INTEGER}                            {
  char *tmp = malloc(yyleng+1);
  strncpy(tmp, yytext, yyleng);
  tmp[yyleng] = '\0';
  yylval.val = atoi(tmp);
  free(tmp);
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += strlen(yytext);
  yylloc.last_column  = parser_column_num - 1;
  return LEOP_INTEGER;
}

".."                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num+1;
  parser_column_num+=2;
  return LEOP_RANGE;
}
"{"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return '{';
}
"}"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return '}';
}
":"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return ':';
}
";"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return ';';
}
","                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return ',';
}
"["                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return '[';
}
"]"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return ']';
}
"("                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return '(';
}
")"                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
  return ')';
}
\/\/.*                              {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += strlen(yytext);
  yylloc.last_column  = parser_column_num - 1;
}
"#".*                               {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += strlen(yytext);
  yylloc.last_column  = parser_column_num - 1;
}
[ ]                                 {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_column_num++;
}
[\n]                                {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  yylloc.last_column  = parser_column_num;
  parser_line_num++;
  parser_column_num=0;
}
[\t]                                {
  yylloc.first_line   = parser_line_num;
  yylloc.first_column = parser_column_num;
  yylloc.last_line    = parser_line_num;
  parser_column_num += 8 - parser_column_num % 8;
  yylloc.last_column  = parser_column_num - 1;
}
. {
  fflush(stdout);
  fprintf(stderr, "invalid token at %s:%d:%d\n", parser_filename, parser_line_num, parser_column_num);
  TBX_FAILURE("lexical error");
}
%%
//.                                   {
//  yylloc.first_line   = parser_line_num;
//  yylloc.first_column = parser_column_num;
//  yylloc.last_line    = parser_line_num;
//  yylloc.last_column  = parser_column_num;
//  parser_column_num++;
//  yylval.str = "unknown token";
//  return yytext[0];
//}
