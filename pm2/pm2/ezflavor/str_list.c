
#include <stdio.h>

#include "parser.h"
#include "trace.h"
#include "str_list.h"

char *string_new(char *str)
{
  char *copy = g_malloc(strlen(str)+1);

  strcpy(copy, str);

  return copy;
}

GList *string_list_from_parser(void)
{
  GList *l = NULL;

  for(;;) {
    token_t tok;

    tok = parser_next_token();

    if(tok == END_OF_INPUT)
      break;

    if(tok != IDENT_TOKEN)
      parser_error();

    l = g_list_append(l, (gpointer)string_new(parser_token_image()));
  }
  return l;
}

static void delete_string(gpointer ptr, gpointer usr)
{
  g_free((char *)ptr);
}

void string_list_destroy(GList **l)
{
  if(*l) {
    g_list_foreach(*l, delete_string, NULL);
    g_list_free(*l);
    *l = NULL;
  }
}
