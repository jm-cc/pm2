
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
