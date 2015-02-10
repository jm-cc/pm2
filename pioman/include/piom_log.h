/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_LOG_H
#define PIOM_LOG_H

/** display message in verbose mode, not in quiet mode */
#define PIOM_DISP(str, ...) do { if(getenv("PIOM_VERBOSE") != NULL) fprintf(stderr, "# pioman: " str , ## __VA_ARGS__); } while(0)

/** warning message, always displayed */
#define PIOM_WARN(str, ...) fprintf(stderr, "# pioman: WARNING- " str , ## __VA_ARGS__)

/** fatal error */
#define PIOM_FATAL(str, ...) do { fprintf(stderr, "# pioman: FATAL- " str , ## __VA_ARGS__); abort(); } while(0)



#endif
