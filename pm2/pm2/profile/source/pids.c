/*	pids.c

	Copyright (C) 2003 Samuel Thibault -- samuel.thibault@fnac.net

*/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include "fkt-tools.h"
#include "pids.h"

struct pidentry {
	int pid;
	char name[MAXCOMMLEN+1];
	struct pidentry *next;
};

/* names are hashed according to the first 5 low bits of the 2 first characters,
 * i.e. 1024 entries */
#define NBHASHENTRIES	256
static struct pidentry *pidentries[NBHASHENTRIES];

/* add a pid name, return 1 if new */
int add_pid( int pid, char name[MAXCOMMLEN+1] )
	{
	struct pidentry *pidentry,*hashed;

	for( hashed=pidentries[pid%NBHASHENTRIES];
					hashed && hashed->pid != pid;
					hashed=hashed->next );
	if( hashed )
		{
		memcpy(hashed->name,name,MAXCOMMLEN+1);
		return 0;
		}

	if( !(pidentry=malloc(sizeof(*pidentry))) )
		{
		perror("couldn't allocate room for new pid");
		exit(EXIT_FAILURE);
		}

	memcpy(pidentry->name,name,MAXCOMMLEN+1);
	pidentry->pid=pid;
	pidentry->next=pidentries[pid%NBHASHENTRIES];
	pidentries[pid%NBHASHENTRIES]=pidentry;
	return 1;
	}

void remove_pid( int pid )
	{
	struct pidentry **entry;
	for(entry=&pidentries[pid%NBHASHENTRIES]; *entry && (*entry)->pid != pid;
			entry=&(*entry)->next);
	if( !*entry )
		{
		EPRINTF("unknown pid %u\n",pid);
		return;
		}
	*entry=(*entry)->next;
	}

void record_pids( int fd )
	{
	const int		zero=0;
	int			pid,readpid;
	DIR				*dir;
	FILE			*file;
	struct dirent	*dirent;
	char			*ptr;
	char			name[strlen("/proc/")+5+strlen("/stat")+1]="/proc";
	char			pidname[MAXCOMMLEN+2+1];
	int				res;

	if( !(dir = opendir(name)) )
		{
		perror("opening /proc");
		exit(EXIT_FAILURE);
		}

	name[strlen("/proc")]='/';
	while( (dirent = readdir(dir)) )
		{
		pid = strtol(dirent->d_name, &ptr, 0);
		if( ptr && !*ptr )
			{/* ok, was fully a number */
			name[strlen("/proc/")]='\0';
			strcat(name+strlen("/proc/"),dirent->d_name);
			strcat(name+strlen("/proc/"),"/stat");
			if( !(file = fopen(name,"r")) )
				continue; /* may happen if it just died */
			res = fscanf(file,"%d %s",&readpid,pidname);
			fclose(file);
			if( res < 2 )
				continue;
			*(strchr(pidname,')')) = '\0';
			if( readpid!=pid )
				{/* uh? */
				fprintf(stderr,"bad pid %d in %s\n",readpid,name);
				continue;
				}
			write(fd,&pid,sizeof(pid));
			write(fd,pidname+1,MAXCOMMLEN+1);
			}
		}
	/* end of list is a zero pid */
	write(fd,&zero,sizeof(zero));
	}

void load_pids( int fd )
	{
	static int		i;
	struct pidentry	*entry;
	if( !(entry = malloc(sizeof(*entry))) )
		{
		perror("allocating pid entry");
		exit(EXIT_FAILURE);
		}
	if( read(fd, &entry->pid, sizeof(entry->pid))
								< sizeof(entry->pid) )
		{
		perror("reading pid");
		exit(EXIT_FAILURE);
		}
	if( !entry->pid )
		{
		free(entry);
		return;
		}
	i++;
	fprintf(stderr,"\r%d",i);
	if( read(fd, &entry->name, sizeof(entry->name))
							< sizeof(entry->name) )
		{
		perror("reading pid name");
		exit(EXIT_FAILURE);
		}

	entry->name[MAXCOMMLEN]='\0';
	entry->next=pidentries[entry->pid%NBHASHENTRIES];
	pidentries[entry->pid%NBHASHENTRIES]=entry;
	load_pids(fd);
	}

char *lookup_pid( int pid )
	{
	struct pidentry	*cur;
	if( pid <= 0 ) return "idle";
	for( cur=pidentries[pid%NBHASHENTRIES];
		cur && cur->pid != pid; cur=cur->next);
	if( !cur ) return "unknown";
	else return cur->name;
	}

/* vim: ts=4
 */
