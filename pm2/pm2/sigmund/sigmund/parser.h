#ifndef PARSER
#define PARSER

#include "sigmund.h"

/* Initializes a new parser that will use 'fd' as its input */
void parser_start(int fd);

/* deletes the parsing buffers and closes 'fd' */
void parser_stop();

void parser_run(void);

#endif
