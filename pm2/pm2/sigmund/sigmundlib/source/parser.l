%{
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "parser.h"
%}

%%
\{[ ]*0x[0-9abcdef]+[ ]*,[ ]*\"[-_a-zA-Z0-9]*\"[ ]*\}, {
     char *s;
     s = strtok(yytext + 1, ",");
     sscanf(s, "%x", &code_table[nb_code].code);
     s = strtok(NULL, "\"");
     s = strtok(NULL, "\"");
     code_table[nb_code].name = (char *) calloc(sizeof(char), 200);
     assert(code_table[nb_code].name != NULL);
     code_table[nb_code].name[199] = '\0';
     sscanf(s, "%s", code_table[nb_code].name);
     code_table[nb_code].name[strlen(code_table[nb_code].name)] = '\0';
     nb_code++;
  }
[\t\n] ;
. ;
%%

static FILE *the_file;

void parser_start(int fd)
{
  the_file = fdopen(fd, "r");
  if(the_file == NULL) {
    perror("fdopen");
    exit(1);
  }
  yy_delete_buffer(YY_CURRENT_BUFFER);
  yy_switch_to_buffer(yy_create_buffer(the_file, 2048));
}

void parser_stop(void)
{
  fclose(the_file);
}

void parser_run(void)
{
  yylex();
}
