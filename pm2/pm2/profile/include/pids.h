/*	pids.h

	Copyright (C) 2003 Samuel Thibault -- samuel.thibault@fnac.net

*/

#define MAXCOMMLEN 15

int add_pid( int pid, char name[MAXCOMMLEN+1] );
void remove_pid( int pid );
void record_pids( int fd );
void load_pids( int fd );
char *lookup_pid( int pid );
