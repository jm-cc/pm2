/*	sysmap.c

	Copyright (C) 2003 Samuel Thibault -- samuel.thibault@fnac.net

*/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <bfd.h>
#include "fkt-tools.h"
#include "sysmap.h"

#define MAXNAMELEN 128

/* System.map is sorted, but /proc/ksyms is not, so we have to sort out a little
 * bit ourselves */

struct sysmapentry {
	char *name;
	int len;
	unsigned long address;
	sysmap_source source;
	struct sysmapentry *next;
};

struct moduleentry {
	char *name;
	unsigned long base;
	const char *path;
	struct moduleentry *next;
};

/* names are hashed according to the first 5 low bits of the 2 first characters,
 * i.e. 1024 entries */
#define NBHASHENTRIES	1024
static struct sysmapentry *sysmapentries[NBHASHENTRIES];
static struct moduleentry *moduleentries;

/* add a symbol, return 1 if new */
int add_symbol( struct sysmapentry *entry )
	{
	struct sysmapentry *cur,*found=NULL;
	struct sysmapentry **hash;
	unsigned long hashkey;
	const char *c;
	
	for( hashkey=0, c=entry->name; *c; hashkey+=*c, c++);
	hash = &sysmapentries[hashkey%NBHASHENTRIES];

	for( cur = *hash; cur; cur = cur->next )
		if( !strcmp(cur->name,entry->name) && cur->source!=entry->source
			&& (!found || found->address != entry->address) )
		/* if the same name is found in another source, and we didn't already,
		 * or with another address, update the found pointer */
			found = cur;
	if( found )
		{
		if( found->address != entry->address )
			{/* different address and built-in */
			fprintf(stderr,"incoherent symbol %s: %lx and %lx\n", entry->name,
												entry->address, found->address);
			fprintf(stderr,"please provide the /boot/System.map file of the current running kernel\nthanks to the -S option\n");
			exit(EXIT_FAILURE);
			}
		return 0;
		}
	else
		{
		entry->next = *hash;
		*hash = entry;
		return 1;
		}
	}

static void get_sysmapfd( FILE *fd, sysmap_source source, unsigned long base);

static void add_elfsymbols( const char *path, unsigned long base )
	{
	static int			bfd_inited = 0;
	bfd					*abfd;
	long				symcount;
	void				*minisyms,*curminisym;
	unsigned int		size;
	asymbol				*store,*cur;
	struct sysmapentry	*entry;

	if (!bfd_inited)
		{
		bfd_init();
		bfd_set_default_target("");
		bfd_inited=1;
		}
	if (!(abfd = bfd_openr(path,NULL)))
		/* bad target */
		goto out;
	if (!bfd_check_format(abfd,bfd_object))
		goto out_bfd;
	if (!(bfd_get_file_flags(abfd)&HAS_SYMS))
		goto out_bfd;
	if ((symcount=bfd_read_minisymbols(abfd, 0, &minisyms, &size)) <= 0)
		/* no symbols or error */
		goto out_bfd;

	if (!(store=bfd_make_empty_symbol(abfd)))
		goto out_minisym;

	for (curminisym = minisyms;
		curminisym < minisyms + symcount*size; 
		curminisym += size)
		{
		if (!(cur = bfd_minisymbol_to_symbol(abfd, 0, curminisym, store)))
			{
			bfd_perror("bfd_minisymbol_to_symbol");
			exit(EXIT_FAILURE);
			}
		if (!cur->value || !cur->name || !*(cur->name) || !(cur->flags&BSF_FUNCTION))
			continue;
		if( !(entry = malloc(sizeof(*entry))) )
			{
			fprintf(stderr,"couldn't allocate memory for symbol\n");
			perror("malloc");
			exit(EXIT_FAILURE);
			}
		entry->source = SYSMAP_FILE;
		entry->name = strdup(cur->name);

		entry->len = strlen(cur->name);
		entry->address = cur->value + cur->section->vma + base;
		//printf("%s(%p:%s): %#010llx\n+%#010llx\n+%#010lx\n=%#010lx\n",entry->name,cur->section,cur->section->name,cur->value,cur->section->vma,base,entry->address);

		if (!add_symbol(entry))
			free(entry);
		}

out_minisym:
	free(minisyms);
out_bfd:
	bfd_close(abfd);
out:
	return;
	}

void get_mysymbols( void )
	{
	FILE *maps;
	char attrs[5];
	char path[256];
	unsigned long base,size,offset;
	int n;
	int first=1;
	if (!(maps=fopen("/proc/self/maps","r")))
		{
		perror("opening /proc/self/maps");
		exit(EXIT_FAILURE);
		}
	while (1)
		{
		if ((n=(fscanf(maps,"%lx-%*s%s%lx%*s%ld",&base,attrs,&offset,&size)))==EOF)
			break;
		if (n==4 && size)
			{
			if ((n=fscanf(maps,"%s",path))==EOF)
				break;
			if (n==1 && attrs[2]=='x')
				add_elfsymbols(path,first?0:base+offset);
			if (first) first=0;
			}
		do { n=fgetc(maps); }
		while( n!=EOF && n!='\n' );
		}
	fclose(maps);
	}

void addmodule_symbols( struct moduleentry *module )
	{
	int fd[2];
	FILE *ffd;
	char *d;
	pid_t				pid;
	/* try to nm the module */
	if( (d = strstr(module->path,".o_M")) )
		*(d+2)='\0';	/* strip module version */

	fprintf(stderr,"\rnming %s\n",module->path);
	if( pipe(fd) )
		{
		perror("couldn't make a pipe for nm\n");
		exit(EXIT_FAILURE);
		}
	if( !(pid=fork()) )
		{/* exec nm on this file */
		close(fd[0]);
		close(STDOUT_FILENO);
		if( dup2(fd[1],STDOUT_FILENO) < 0 )
			{
			perror("couldn't redirect output of nm\n");
			exit(EXIT_FAILURE);
			}
		close(fd[1]);
		execlp("nm", "nm", module->path, NULL);
		fprintf(stderr,"couldn't launch nm on %s\n",module->path);
		perror("execlp");
		exit(EXIT_FAILURE);
		}

	close(fd[1]);
	if( !(ffd=fdopen(fd[0],"r")) )
		/* couldn't open nm output, give up */
		kill(pid,SIGTERM);
	else
		get_sysmapfd(ffd, SYSMAP_FILE, module->base);
	wait(NULL);
	}

static void get_sysmapfd( FILE *fd, sysmap_source source, unsigned long base)
	{
	int					i=0,j,n;
	char				*c;
	struct sysmapentry	*entry=NULL;
	struct moduleentry	*module;
	char				*name;
	unsigned long		address;
	int					len;
	char				type;
	
	for(;;i++)
		{
		fprintf(stderr,"\r%d",i);
		if( !(name = malloc(MAXNAMELEN)) )
			{
			fprintf(stderr,"couldn't allocate memory for name\n");
			perror("malloc");
			exit(EXIT_FAILURE);
			}

		type=0;
		if( (n = fscanf(fd, "%lx ", &address)) == EOF
			|| (n > 0 && (n = fscanf(fd, "%s%n", name, &len)) == EOF)
			|| (n > 0 && len<=1 && (type=name[0],
					n = fscanf(fd, "%s%n", name, &len)) == EOF ) )
			{/* end of file, stop */
			fprintf(stderr," done\n");
			return;
			}

		do { j=fgetc(fd); }
		while( j!=EOF && j!='\n' );

		if( !n || (type != 0 && type != 't' && type != 'T') )
			/* garbage or non text, continue */
			{
			free(name);
			continue;
			}

		for( c=name; c<name+len && *c!=' '; c++);
		len=c-name;
		name[len]='\0';

		if( source != SYSMAP_MODLIST )
			{
			if( !(entry = malloc(sizeof(*entry))) )
				{
				fprintf(stderr,"couldn't allocate memory for symbol\n");
				perror("malloc");
				exit(EXIT_FAILURE);
				}
			entry->source = source;
			entry->name = name;
			entry->len = len;
			entry->address = address + base;
		}
		
		/* look for module insertion tags */
		if( !strncmp(name,"__insmod_",strlen("__insmod_")) )
			{
			if( source != SYSMAP_MODLIST )
				{/* but ignore them if not a module list */
				free(name);
				continue;
				}
			if( (c = strstr(name,"_O/")) )
				{/* module load tag, record it */
				/* look for text address to add the module path */
				for( module=moduleentries; module &&
							(strncmp(module->name,name,c-name)
							||strncmp(module->name+(c-name),"_S.text_L",
									strlen("_S.text_L")));
							module = module->next);
				if( module )
					{
					free(module->name);
					module->name = name + strlen("__insmod_");
					module->path = c+2;
					addmodule_symbols(module);
					}
				else
					{
					if( !(module=malloc(sizeof(*module))) )
						{
						perror("couldn't allocate module entry\n");
						exit(EXIT_FAILURE);
						}
					module->name = name;
					module->path = c+2;
					module->next = moduleentries;
					moduleentries = module;
					}
				}
			else if( (c = strstr(name,"_S.text_L")) )
				{/* module text address */
				/* look for module load tag to add the base address */
				for( module = moduleentries; module &&
							(strncmp(module->name,name,c-name)
							||strncmp(module->name+(c-name),"_O/",
									strlen("_O/")));
							module = module->next);
				if( module )
					{
					module->base = address;
					free(name);
					addmodule_symbols(module);
					}
				else
					{
					if( !(module=malloc(sizeof(*module))) )
						{
						perror("couldn't allocate module entry\n");
						exit(EXIT_FAILURE);
						}
					module->name = name;
					module->base = address;
					module->next = moduleentries;
					moduleentries = module;
					}
				}
			}
		else
			/* only add symbol if not module list */
			if( source != SYSMAP_MODLIST && !add_symbol(entry) )
				free(entry);
		}
	}

void get_sysmap( char *systemmap, sysmap_source source, unsigned long base )
	{
	FILE *fd;
	if( !systemmap ) return;
	fprintf(stderr,"opening %s\n", systemmap);
	if( (fd = fopen(systemmap, "r")) < 0 )
		{
		fprintf(stderr,"opening %s\n",systemmap);
		perror("open");
		exit(EXIT_FAILURE);
		}
	get_sysmapfd( fd, source, base );
	fclose(fd);
	}

void record_sysmap( int fd )
	{
	int i;
	const unsigned long zero=0;
	struct sysmapentry *cur;
	for( i=0; i<NBHASHENTRIES; i++ )
		for( cur = sysmapentries[i]; cur; cur=cur->next )
			{
			if (cur->address<0x08000000)
				fprintf(stderr,"strange symbol %s: %lx\n",cur->name,cur->address);
			else
				{
				write(fd,&cur->address,sizeof(cur->address));
				write(fd,&cur->len,sizeof(cur->len));
				write(fd,cur->name,cur->len);
				}
			}
	/* end of list is a zero address */
	write(fd,&zero,sizeof(zero));
	}

void load_sysmap( int fd )
	{
	static int i;
	struct sysmapentry	*entry;
	if( !(entry = malloc(sizeof(*entry))) )
		{
		perror("allocating System map entry");
		exit(EXIT_FAILURE);
		}
	if( read(fd, &entry->address, sizeof(entry->address))
								< sizeof(entry->address) )
		{
		perror("reading System map address");
		exit(EXIT_FAILURE);
		}
	if( !entry->address )
		{
		free(entry);
		return;
		}
	if (!((i++)%1024))
		fprintf(stderr,"\r%d",i);
	if( read(fd, &entry->len, sizeof(entry->len))
							< sizeof(entry->len) )
		{
		perror("reading System map address");
		exit(EXIT_FAILURE);
		}
	if( !(entry->name = malloc(entry->len+1)) )
		{
		perror("allocating System map entry name");
		exit(EXIT_FAILURE);
		}
	if( read(fd, entry->name, entry->len) < entry->len )
		{
		perror("reading System map name");
		exit(EXIT_FAILURE);
		}

	entry->name[entry->len]='\0';
	//printf("%s: %#lx\n",entry->name,entry->address);
	entry->next=sysmapentries[entry->address%NBHASHENTRIES];
	sysmapentries[entry->address%NBHASHENTRIES]=entry;
	load_sysmap(fd);
	}

char *lookup_symbol( unsigned long address )
	{
	struct sysmapentry	*cur;
	for( cur=sysmapentries[address%NBHASHENTRIES];
		cur && cur->address != address; cur=cur->next);
	if( !cur ) return NULL;
	return cur->name;
	}

/* vim: ts=4
 */
