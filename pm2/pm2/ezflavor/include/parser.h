
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

#ifndef PARSER
#define PARSER

#include "shell.h"

typedef enum {
  END_OF_INPUT,
  DASH_DASH_TOKEN,
  EQUAL_TOKEN,
  IDENT_TOKEN,
  STRING_TOKEN,
  UNKNOWN_TOKEN
} token_t;

/* Initializes a new parser that will use 'fd' as its input */
void parser_start(int fd);

void parser_start_cmd(char *fmt, ...);

/* deletes the parsing buffers and closes 'fd' */
void parser_stop();

token_t parser_next_token(void);

char *parser_token_image(void);

void parser_error(void);

#endif
