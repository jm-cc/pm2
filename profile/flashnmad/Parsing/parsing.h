#ifndef _PARSING_H
#define _PARSING_H

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fxt/fut.h>
#include <fxt/fxt.h>
#include <search.h>
#include "nmad_fxt.h"
#include "nmad_state.h"
#include "NMadState_ops.h"

void process_log(nmad_state_t *nmad_status, char *pathtolog);
void preprocess_log(nmad_state_t *nmad_status, char *pathtolog);

#endif /* _PARSING_H */
