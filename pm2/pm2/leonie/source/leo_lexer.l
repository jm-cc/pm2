%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #define DEBUG */
#include "leonie.h"
#undef DEBUG
#define YY_INPUT(buf, result, max_size) \
   {result = leo_yy_input(buf, max_size);}
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
STRING         \".+\"
ID             [_a-zA-Z][_a-zA-Z0-9\-]*
EID            {ID}"."{ID}("."{ID})*
%%
alias                               {
  LOG("<ALIAS>");
  return LEOP_ALIAS;
}
adapter                             {
  LOG("<ADAPTER>");
  return LEOP_ADAPTER;
}
adapters                            {
  LOG("<ADAPTERS>");
  return LEOP_ADAPTERS;
}
archi                               {
  LOG("<ARCHI>");
  return LEOP_ARCHI;
}
application                         {
  LOG("<APPLICATION>");
  return LEOP_APPLICATION; 
}
channels                            {
  LOG("<CHANNELS>");
  return LEOP_CHANNELS; 
}
cluster                             {
  LOG("<CLUSTER>");
  return LEOP_CLUSTER; 
}
domain                              {
  LOG("<DOMAIN>");
  return LEOP_DOMAIN; 
}
executable                          {
  LOG("EXECUTABLE");
  return LEOP_EXECUTABLE;
}
host                                { 
  LOG("<HOST>");
  return LEOP_HOST; 
}
hosts                               { 
  LOG("<HOSTS>");
  return LEOP_HOSTS; 
}
loader                              {
  LOG("<LOADER>");
  return LEOP_LOADER;
}
macro                               {
  LOG("<MACRO>");
  return LEOP_MACRO;
}
makefile                            {
  LOG("<MAKEFILE>");
  return LEOP_MAKEFILE;
}
model                               {
  LOG("<MODEL>");
  return LEOP_MODEL;
}
module                              {
  LOG("<MODULE>");
  return LEOP_MODULE;
}
name                                {
  LOG("<NAME>");
  return LEOP_NAME;
}
nfs                                 {
  LOG("<NFS>");
  return LEOP_NFS;
}
no                                  {
  LOG("<NO>");
  return LEOP_NO;
}
none                                {
  LOG("<NONE>");
  return LEOP_NONE;
}
os                                  {
  LOG("<OS>");
  return LEOP_OS;
}
path                                {
  LOG("PATH");
  return LEOP_PATH;
}
protocol                            {
  LOG("<PROTOCOL>");
  return LEOP_PROTOCOL; 
}
register                            {
  LOG("<REGISTER>");
  return LEOP_REGISTER; 
}
rule                                {
  LOG("RULE");
  return LEOP_RULE;
}
selector                            {
  LOG("<SELECTOR>");
  return LEOP_SELECTOR;
}
suffix                              {
  LOG("<SUFFIX>");
  return LEOP_SUFFIX;
}
yes                                 {
  LOG("<YES>");
  return LEOP_YES;
}
{ID}                                {
  yylval.str = malloc(yyleng + 1);
  strcpy(yylval.str, yytext);
  LOG_STR("ID", yylval.str);
  return LEOP_ID; 
}
{EID}                               { 
  yylval.str = malloc(yyleng + 1);
  strcpy(yylval.str, yytext);
  LOG_STR("EID", yylval.str);
  return LEOP_EID; 
}
{STRING}                            { 
  yylval.str = malloc(yyleng + 1);
  strcpy(yylval.str, yytext);
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
