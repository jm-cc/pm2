
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "profile.h"

#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_entries.h"
#endif

#ifdef PM2
#include "pm2.h"
#endif

#define PROF_BUFFER_SIZE  (1024*1024)

char PROF_FILE[1024];

void profile_init(void)
{
  static unsigned already_called = 0;

  if(!already_called) {
#ifdef PM2
    sprintf(PROF_FILE, "/tmp/prof_file_%d", pm2_self());
#else
    strcpy(PROF_FILE, "/tmp/prof_file");
#endif

    if(fut_setup(PROF_BUFFER_SIZE, 0) < 0) {
      perror("fut_setup");
      exit(EXIT_FAILURE);
    }
    already_called = 1;
  }
}

void profile_activate(int how, unsigned keymask)
{
  fut_keychange(how, keymask);
}

void profile_stop(void)
{
  fut_endup(PROF_FILE);
}

