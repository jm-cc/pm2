
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "trace.h"
#include "str_list.h"

char *string_new(const char *str)
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
