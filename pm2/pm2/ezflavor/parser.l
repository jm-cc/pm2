
%{
#include <stdarg.h>
#include <ctype.h>

#include "parser.h"
%}

%%
"="[^ \n\t]*	                { return EQUAL_TOKEN; }
"--"[-A-Za-z0-9_]*              { return DASH_DASH_TOKEN; }
[A-Za-z0-9][-A-Za-z0-9_]*	{ return IDENT_TOKEN; }
\'.*\'	|
\".*\"		                { return STRING_TOKEN; }
[ \t\n]			        ;
.			        { return UNKNOWN_TOKEN; }
%%

static FILE *the_file;
static token_t last_token;
static pid_t pid = -1;

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

void parser_start_cmd(char *fmt, ...)
{
  static char cmd[4096];
  char *argv[128];
  va_list vl;
  unsigned nb = 0, i = 0;
  int fd;

  va_start(vl, fmt);
  vsprintf(cmd, fmt, vl);
  va_end(vl);

  for(;;) {
    while(cmd[i] != '\0' && isspace(cmd[i]))
      i++;

    if(cmd[i] == '\0')
      break;

    argv[nb++] = &cmd[i];

    while(cmd[i] != '\0' && !isspace(cmd[i]))
      i++;

    if(cmd[i] == '\0')
      break;

    cmd[i++] = '\0';
  }

  argv[nb] = NULL;

  pid = exec_single_cmd(&fd, argv);

  parser_start(fd);
}

void parser_stop(void)
{
  fclose(the_file);

  if(pid != -1) {
    exec_wait(pid);
    pid = -1;
  }
}

token_t parser_next_token(void)
{
  last_token = (token_t)yylex();
  return last_token;
}

char *parser_token_image(void)
{
  return yytext;
}

void parser_error(void)
{
  fprintf(stderr, "Parse error: Unexpected token %d (%s).\n",
	  last_token, parser_token_image());
  exit(1);
}
