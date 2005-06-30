#ifndef PARSER
#define PARSER

#include "sigmund.h"

/* Initializes a new parser that will use 'fd' as its input */
extern void parser_start(int fd);

/* deletes the parsing buffers and closes 'fd' */
extern void parser_stop();

extern void parser_run(void);

#endif
