%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "leoparse.h"
#undef DEBUG
#define YY_INPUT(buf, result, max_size) \
   {result = leoparse_yy_input(buf, max_size);}
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
WD             [_a-zA-Z][_a-zA-Z0-9\-]*
ID             {WD}("."{WD})*
%%
{ID}                               { 
  yylval.str = malloc(yyleng + 1);
  strcpy(yylval.str, yytext);
  LOG_STR("ID", yylval.str);
  return LEOP_ID; 
}
{STRING}                            { 
  yylval.str = malloc(yyleng - 1);
  strncpy(yylval.str, yytext + 1, yyleng - 2);
  yylval.str[yyleng - 1] = '\0';
  LOG_STR("STRING", yylval.str);
  return LEOP_STRING; 
}
"{"                                 {
  LOG("<{>");
  return '{'; 
}
"}"                                 {
  LOG("<}>");
  return '}'; 
}
":"                                 {
  LOG("<:>");
  return ':'; 
}
"="                                 {
  LOG("<=>");
  return '='; 
}
";"                                 {
  LOG("<;>");
  return ';'; 
}
","                                 { 
  LOG("<,>");
  return ','; 
}
\/\/.*                              ;
"#".*                               ;
[ \n\t]                             ;
.                                   { 
  LOG("Inconnu !!!");
  return INCONNU;
}
%%
