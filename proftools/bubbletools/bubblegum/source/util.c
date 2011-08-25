/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

#include "actions.h"
#include "util.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

static char tmpDirectory[STRING_BUFFER_SIZE];

void create_tmp_directory() {
  snprintf(tmpDirectory, sizeof(tmpDirectory), "/tmp/bubblegum_user_%s", getenv ("USER"));
  mkdir(tmpDirectory, 0775);
}

void remove_tmp_directory() {
  char command[STRING_BUFFER_SIZE+10];
  sprintf(command, "rm -rf %s", tmpDirectory);
  system(command);
}

void get_tmp_bubblegum_file(unsigned int num, char **path) {
  sprintf(*path, "%s/bubblegumT%d.xml", tmpDirectory, num);
}
