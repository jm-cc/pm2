
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2004 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#ifdef MA__NUMA
#include <math.h>
#endif

/* cpus */
#ifdef MA__NUMA
#include <sched.h>
#ifdef CPU_SET
typedef cpu_set_t ma_cpu_set_t;
#define MA_CPU_SET(cpu, cpusetp)	CPU_SET(cpu, cpusetp)
#define MA_CPU_CLR(cpu, cpusetp)	CPU_CLR(cpu, cpusetp)
#define MA_CPU_ISSET(cpu, cpusetp)	CPU_ISSET(cpu, cpusetp)
#define MA_CPU_ZERO(cpusetp)		CPU_ZERO(cpusetp)
#else
typedef unsigned long ma_cpu_set_t;
#define MA_CPU_MASK(cpu)		(1UL<<(cpu))
#define MA_CPU_SET(cpu, cpusetp)	(*(cpusetp) |=  MA_CPU_MASK(cpu))
#define MA_CPU_CLR(cpu, cpusetp)	(*(cpusetp) &= ~MA_CPU_MASK(cpu))
#define MA_CPU_ISSET(cpu, cpusetp)	(*(cpusetp) &   MA_CPU_MASK(cpu))
#define MA_CPU_ZERO(cpusetp)		(*(cpusetp) = 0UL)
#endif
#endif

unsigned marcel_topo_nblevels=
#ifndef MA__LWPS
	0
#else
	1
#endif
;

struct marcel_topo_level marcel_machine_level[] = {
	{
		.type = MARCEL_LEVEL_MACHINE,
		.number = 0,
		.index = 0,
		.vpset = MARCEL_VPMASK_FULL,
		.arity = 0,
		.sons = NULL,
		.father = NULL,
#ifdef MARCEL_SMT_IDLE
		.nbidle = MA_ATOMIC_INIT(0),
#endif
		.sched = NULL,
	},
	{
		.vpset = MARCEL_VPMASK_EMPTY,
	}
};


struct marcel_topo_level *marcel_topo_levels[2*MARCEL_LEVEL_LAST+1] = {
	marcel_machine_level
};

#ifdef MA__LWPS

static int discovering_level = 1;
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 1;
unsigned marcel_lwps_per_cpu = 1;
#ifdef MA__NUMA
unsigned marcel_topo_max_arity = 0;
#endif

void ma_set_nbprocessors(void) {
	// Détermination du nombre de processeurs disponibles
	marcel_nbprocessors = ma_nbprocessors();
	mdebug("%d processors available\n", marcel_nbprocessors);
}

void ma_set_processors(void) {
	marcel_lwps_per_cpu = (get_nb_lwps()+marcel_nbprocessors-1)/marcel_nbprocessors;
	mdebug("%d LWP%s per cpu, stride %d\n", marcel_lwps_per_cpu, marcel_lwps_per_cpu == 1 ? "" : "s", marcel_cpu_stride);
	// TODO: stride = 2 si HT et que suffisament peu de LWPs.
}

#ifdef MA__NUMA

#ifdef MARCEL_SMT_IDLE
static struct marcel_topo_level *marcel_topo_core_level;
#endif
static struct marcel_topo_level *marcel_topo_node_level;
static struct marcel_topo_level *marcel_topo_cpu_level;

int ma_lwp_node[MA_NR_LWPS];

#ifdef LINUX_SYS
#define PROCESSOR	"processor\t: "
#define PROCESSOR2	"processor  : "
#define PHYSID		"physical id: "
#define COREID		"core id\t: "
#define COREID2		"core id    : "
//#define THREADID	"thread id  : "

static void __marcel_init look_cpuinfo(void) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	long processor=-1, physid, maxphysid=-1, coreid, maxcoreid=-1;
	int i,j,k;
	unsigned proc_physid[MARCEL_NBMAXCPUS],proc_coreid[MARCEL_NBMAXCPUS];

	unsigned cpu, cpu2;

	int dienum[maxphysid+1];
	ma_cpu_set_t diecpus[maxphysid+1];
	struct marcel_topo_level *die_level;
	unsigned numdies=0;
	int really_dies=0;

	int corenum[numdies*(maxcoreid+1)];
	ma_cpu_set_t corecpus[numdies*(maxcoreid+1)];
	struct marcel_topo_level *core_level;
	unsigned numcores=0;
	int really_cores=0;

	if (!(fd=fopen("/proc/cpuinfo","r"))) {
		fprintf(stderr,"could not open /proc/cpuinfo\n");
		return;
	}

	memset(proc_physid,0,sizeof(proc_physid));
	memset(proc_coreid,0,sizeof(proc_coreid));

	while (fgets(string,sizeof(string),fd)!=NULL) {
#define getprocnb_begin(field, var) \
		if ( !strncmp(field,string,strlen(field))) { \
			var = strtol(string+strlen(field),&endptr,0); \
			if (endptr==string+strlen(field)) { \
				fprintf(stderr,"no number in "field" field of /proc/cpuinfo\n"); \
				break; \
			} else if (var==LONG_MIN) { \
				fprintf(stderr,"too small "field" number in /proc/cpuinfo\n"); \
				break; \
			} else if (processor==LONG_MAX) { \
				fprintf(stderr,"too big "field" number in /proc/cpuinfo\n"); \
				break; \
			} \
			mdebug(field " %ld\n", var)
#define getprocnb_end() \
		}
		getprocnb_begin(PROCESSOR,processor);
		getprocnb_end() else
		getprocnb_begin(PROCESSOR2,processor);
		getprocnb_end() else
		getprocnb_begin(PHYSID,physid);
			proc_physid[processor]=physid;
			if (physid>maxphysid)
				maxphysid=physid;
		getprocnb_end() else
		getprocnb_begin(COREID,coreid);
			proc_coreid[processor]=coreid;
			if (coreid>maxcoreid)
				maxcoreid=coreid;
		getprocnb_end() else
		getprocnb_begin(COREID2,coreid);
			proc_coreid[processor]=coreid;
			if (coreid>maxcoreid)
				maxcoreid=coreid;
		getprocnb_end()
		if (string[strlen(string)-1]!='\n') {
			fscanf(fd,"%*[^\n]");
			getc(fd);
		}
	}
	fclose(fd);

	mdebug("%ld processors\n", processor+1);

	memset(dienum,0,sizeof(dienum));
	memset(diecpus,0,sizeof(diecpus));

	if (maxphysid == -1) {
		numdies = 1;
		proc_physid[0] = maxphysid = 0;
	} else
	/* normalize die numbers */
	for (cpu=0; cpu <= processor; cpu++) {
		if (!dienum[proc_physid[cpu]]) {
			dienum[proc_physid[cpu]] = -(++numdies);
		} else
			really_dies=1;
		MA_CPU_SET(cpu,&diecpus[proc_physid[cpu]]);
		mdebug("%d on die %d\n", cpu, -dienum[proc_physid[cpu]]-1);
	}

	if (numdies>1)
		mdebug("%d dies\n", numdies);

	if (really_dies) {
		MA_BUG_ON(!(die_level=TBX_MALLOC((numdies+1)*sizeof(*die_level))));

		for (cpu=0, j=0; cpu <= processor; cpu++) {
			i = proc_physid[cpu];
			if (dienum[i] < 0) {
				dienum[i] = -dienum[i]-1;
				die_level[j].type = MARCEL_LEVEL_DIE;
				die_level[j].number = j;
				marcel_vpmask_empty(&die_level[j].vpset);
				for (k=0; k < get_nb_lwps(); k++) {
					cpu2 = ma_cpu_of_lwp_num(k);
					if (MA_CPU_ISSET(cpu2,&diecpus[i]))
						marcel_vpmask_add_vp(&die_level[j].vpset,k);
				}
				die_level[j].sched = NULL;
				mdebug("die %d has vpset %lx\n",j,die_level[j].vpset);
				j++;
			}
		}

		marcel_vpmask_empty(&die_level[j].vpset);

		marcel_topo_levels[discovering_level++]=die_level;
	}

	memset(corenum,0,sizeof(corenum));
	memset(corecpus,0,sizeof(corecpus));

	if (maxcoreid == -1) {
		numcores = 1;
		proc_coreid[0] = maxcoreid = 0;
	} else
	/* normalize core numbers */
	for (cpu=0; cpu <= processor; cpu++) {
		physid = proc_physid[cpu];
		i = dienum[physid]+proc_coreid[cpu]*numdies;
		if (!corenum[i])
			corenum[i] = -(++numcores);
		else
			really_cores=1;
		MA_CPU_SET(cpu,&corecpus[i]);
	}

	if (numcores>1)
		mdebug("%d cores\n", numcores);

	if (really_cores) {
		MA_BUG_ON(!(core_level=TBX_MALLOC((numcores+1)*sizeof(*core_level))));

		for (cpu=0, j=0; cpu <= processor; cpu++) {
			physid = proc_physid[cpu];
			i = dienum[physid]+proc_coreid[cpu]*numdies;
			if (corenum[i] < 0) {
				corenum[i] = -corenum[i]-1;
				core_level[j].type = MARCEL_LEVEL_CORE;
				core_level[j].number = j;
				marcel_vpmask_empty(&core_level[j].vpset);
				for (k=0; k < get_nb_lwps(); k++) {
					cpu2 = ma_cpu_of_lwp_num(k);
					if (MA_CPU_ISSET(cpu2,&corecpus[i]))
						marcel_vpmask_add_vp(&core_level[j].vpset,k);
				}
				core_level[j].sched = NULL;
#ifdef MARCEL_SMT_IDLE
				ma_atomic_set(&core_level[j].nbidle, 0);
#endif
				mdebug("core %d has vpset %lx\n",j,core_level[j].vpset);
				j++;
			}
		}

		marcel_vpmask_empty(&core_level[j].vpset);

#ifdef MARCEL_SMT_IDLE
		marcel_topo_core_level =
#endif
		marcel_topo_levels[discovering_level++]=core_level;
	}
}
#endif

#ifdef LINUX_SYS
#include <numa.h>
static void __marcel_init look_libnuma(void) {
	unsigned long *buffer,*buffer2;
	unsigned buffersize=MARCEL_NBMAXCPUS/8;
	unsigned i,j;
	unsigned nbnodes;
	struct marcel_topo_level *node_level;
	marcel_vpmask_t vpset;

	numa_exit_on_error=1;

	if ((ma_numa_not_available = (numa_available())==-1))
		return;

	nbnodes=numa_max_node()+1;
	if (nbnodes==1) {
		nbnodes=0;
		return;
	}

	MA_BUG_ON(nbnodes==0);

	MA_BUG_ON(!(node_level=TBX_MALLOC((nbnodes+1)*sizeof(*node_level))));

	for (i=0;i<get_nb_lwps();i++)
		ma_lwp_node[i]=-1;

	if (!(buffer=TBX_MALLOC(buffersize))) {
		fprintf(stderr,"no room for storing cpu mask\n");
		return;
	}

	while (numa_node_to_cpus(0, buffer, buffersize)==-1 && errno==ERANGE) {
		buffersize*=2;
		if (!(buffer2 = TBX_REALLOC(buffer, buffersize))) {
			fprintf(stderr,"no room for storing cpu mask\n");
			TBX_FREE(buffer);
			return;
		}
		buffer=buffer2;
	}

	for (i=0;i<nbnodes;i++) {
		if (numa_node_to_cpus(i, buffer, buffersize)) {
			fprintf(stderr,"numa_node_to_cpus(%d) failed: %s\n",i,strerror(errno));
			continue;
		}
		node_level[i].type = MARCEL_LEVEL_NODE;
		node_level[i].number=i;
		node_level[i].vpset=vpset=buffer[0];
		node_level[i].sched = NULL;
		mdebug("node %d has vpset %lx\n",i,vpset);
		for (j=0;j<get_nb_lwps();j++)
			if (marcel_vpmask_vp_ismember(&vpset,j))
				ma_lwp_node[j]=i;
	}

	marcel_vpmask_empty(&node_level[i].vpset);

	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;

	TBX_FREE(buffer);
}
#endif


#ifdef OSF_SYS
#include <numa.h>
static void __marcel_init look_libnuma(void) {
	cpu_cursor_t cursor;
	unsigned i,j;
	unsigned nbnodes;
	radid_t radid;
	cpuid_t cpuid;
	cpuset_t cpuset;
	struct marcel_topo_level *node_level;
	marcel_vpmask_t vpset;

	nbnodes=rad_get_num();
	if (nbnodes==1) {
		nbnodes=0;
		return;
	}

	MA_BUG_ON(nbnodes==0);

	MA_BUG_ON(!(node_level=TBX_MALLOC((nbnodes+1)*sizeof(*node_level))));

	for (i=0;i<get_nb_lwps();i++)
		ma_lwp_node[i]=-1;

	cpusetcreate(&cpuset);
	for (radid = 0; radid < nbnodes; radid++) {
		cpuemptyset(cpuset);
		if (rad_get_cpus(radid, cpuset)==-1) {
			fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
			continue;
		}
		
		node_level[radid].type = MARCEL_LEVEL_NODE;
		node_level[radid].number=radid;
		marcel_vpmask_empty(&node_level[radid].vpset);
		cursor = SET_CURSOR_INIT;
		while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
			marcel_vpmask_add_vp(&node_level[radid].vpset,cpuid);
		vpset = node_level[radid].vpset;
		node_level[radid].sched = NULL;
		mdebug("node %d has vpset %lx\n",i,vpset);
		for (j=0;j<get_nb_lwps();j++)
			if (marcel_vpmask_vp_ismember(&vpset,j))
				ma_lwp_node[j]=radid;
	}

	marcel_vpmask_empty(&node_level[i].vpset);

	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;
}
#endif


static void look_cpu(void) {
	struct marcel_topo_level *cpu_level;
	unsigned i,cpu;

	if (marcel_nbprocessors>=get_nb_lwps())
		return;

	MA_BUG_ON(!(cpu_level=TBX_MALLOC((marcel_nbprocessors+1)*sizeof(*cpu_level))));

	for (cpu=0; cpu<marcel_nbprocessors; cpu++) {
		cpu_level[cpu].type=MARCEL_LEVEL_PROC;
		cpu_level[cpu].number=cpu;
		marcel_vpmask_empty(&cpu_level[cpu].vpset);
		cpu_level[cpu].sched=NULL;
	}
	marcel_vpmask_empty(&cpu_level[cpu].vpset);

	for (i=0; i<get_nb_lwps(); i++) {
		cpu = ma_cpu_of_lwp_num(i);
		marcel_vpmask_add_vp(&cpu_level[cpu].vpset,i);
	}

	for (cpu=0; cpu<marcel_nbprocessors; cpu++)
		mdebug("cpu %d has vpset %lx\n",cpu,cpu_level[cpu].vpset);

	marcel_topo_levels[discovering_level++]=
		marcel_topo_cpu_level = cpu_level;
}
#endif /* MA__NUMA */

static void look_lwp(void) {
	struct marcel_topo_level *lwp_level;
	unsigned i;

	if (get_nb_lwps()==1)
		return;

	MA_BUG_ON(!(lwp_level=TBX_MALLOC((get_nb_lwps()+1)*sizeof(*lwp_level))));

	for (i=0; i<get_nb_lwps(); i++) {
		lwp_level[i].type=MARCEL_LEVEL_LWP;
		lwp_level[i].number=i;
		marcel_vpmask_only_vp(&lwp_level[i].vpset,i);
		lwp_level[i].arity=0;
		lwp_level[i].sons=NULL;
		lwp_level[i].sched=NULL;
	}
	marcel_vpmask_empty(&lwp_level[i].vpset);

	marcel_topo_levels[discovering_level++] = lwp_level;
}

#ifdef MA__NUMA
static void split(unsigned arity, unsigned * __restrict sublevelsize, unsigned * __restrict nbsublevels) {
	*sublevelsize = sqrt(arity);
	*nbsublevels = (arity + *sublevelsize-1) / *sublevelsize;
	if (*nbsublevels > marcel_topo_max_arity) {
		*nbsublevels = marcel_topo_max_arity;
		*sublevelsize = (arity + *nbsublevels-1) / *nbsublevels;
	}
}
#endif

static void topo_discover(void) {
	unsigned l,i,j,m;
#ifdef MA__NUMA
	unsigned k;
	unsigned levelsize;
	unsigned nbsublevels;
	unsigned sublevelsize;
	unsigned vp;
	int dosplit;
	struct marcel_topo_level *level;
#endif
#ifdef MA__NUMA
	marcel_vpmask_empty(&marcel_machine_level[0].vpset);
	for (vp=0; vp < get_nb_lwps(); vp++)
		marcel_vpmask_add_vp(&marcel_machine_level[0].vpset, vp);
#ifdef LINUX_SYS
	look_libnuma();
	look_cpuinfo();
#endif
#ifdef OSF_SYS
	look_libnuma();
#endif
	look_cpu();
#endif
	look_lwp();

	marcel_topo_nblevels=discovering_level;

	l = marcel_topo_nblevels-1;
	for (i=0; marcel_topo_levels[l][i].vpset; i++)
		marcel_topo_levels[l][i].type = MARCEL_LEVEL_LWP;

	/* merge identical levels */
	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++);
		for (j=0; j<i && marcel_topo_levels[l+1][j].vpset; j++)
			if (marcel_topo_levels[l+1][j].vpset != marcel_topo_levels[l][j].vpset)
				break;
		if (j==i && !marcel_topo_levels[l+1][j].vpset) {
			/* TODO: merge level types */
			mdebug("merging levels %u and %u since same %d item%s\n", l, l+1, i, i>=2?"s":"");
#ifdef MA__NUMA
			if (marcel_topo_levels[l+1] == marcel_topo_cpu_level)
				marcel_topo_cpu_level = marcel_topo_levels[l];
#ifdef MARCEL_SMT_IDLE
			else if (marcel_topo_levels[l+1] == marcel_topo_core_level)
				marcel_topo_core_level = marcel_topo_levels[l];
#endif
			else if (marcel_topo_levels[l+1] == marcel_topo_node_level)
				marcel_topo_node_level = marcel_topo_levels[l];
#endif
			TBX_FREE(marcel_topo_levels[l+1]);
			memmove(&marcel_topo_levels[l+1],&marcel_topo_levels[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_levels));
			marcel_topo_nblevels--;
			l--;
		}
	}

	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {
			marcel_topo_levels[l][i].arity=0;
			for (j=0; marcel_topo_levels[l+1][j].vpset; j++)
				if (!(marcel_topo_levels[l+1][j].vpset &
					~(marcel_topo_levels[l][i].vpset)))
					marcel_topo_levels[l][i].arity++;
			mdebug("level %u,%u: vpset %lx arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
		}
	}

#ifdef MA__NUMA
	/* Split hi-arity levels */
	if (marcel_topo_max_arity)
		for (l=0; l<marcel_topo_nblevels-1; l++) {
			levelsize = 0;
			dosplit = 0;
			for (i=0; marcel_topo_levels[l][i].vpset; i++) {
				split(marcel_topo_levels[l][i].arity,&sublevelsize,&nbsublevels);
				if (marcel_topo_levels[l][i].arity > marcel_topo_max_arity) {
					mdebug("splitting level %u,%u because %d > %d\n", l, i, marcel_topo_levels[l][i].arity, marcel_topo_max_arity);
					dosplit=1;
				}
				levelsize += nbsublevels;
			}
			if (dosplit) {
				memmove(&marcel_topo_levels[l+2],&marcel_topo_levels[l+1],(marcel_topo_nblevels-(l+1))*sizeof(*marcel_topo_levels));
				marcel_topo_levels[l+1] = TBX_MALLOC((levelsize+1)*sizeof(**marcel_topo_levels));
				for (i=0,j=0; marcel_topo_levels[l][i].vpset; i++) {
					split(marcel_topo_levels[l][i].arity,&sublevelsize,&nbsublevels);
					mdebug("splitting level %u,%u into %u sublevels of size at most %u\n", l, i, nbsublevels, sublevelsize);
					for (k=0; k<nbsublevels; k++) {
						level = &marcel_topo_levels[l+1][j+k];
						level->type = MARCEL_LEVEL_FAKE;
						level->number = j+k;
						level->sched = NULL;
						marcel_vpmask_empty(&level->vpset);
					}
					for (vp=0,k=0,m=0; vp<get_nb_lwps(); vp++) {
						if (marcel_vpmask_vp_ismember(&marcel_topo_levels[l][i].vpset, vp)) {
							marcel_vpmask_add_vp(&marcel_topo_levels[l+1][j+k].vpset, vp);
							marcel_topo_levels[l+1][j+k].arity++;
							if (++m == sublevelsize) {
								/* filled that sublevel, begin next one */
								k++;
								m = 0;
							}
						}
					}
					for (k=0; k<nbsublevels; k++) {
						marcel_topo_levels[l+1][j+k].arity=0;
						for (m=0; marcel_topo_levels[l+2][m].vpset; m++)
							if (!(marcel_topo_levels[l+2][m].vpset &
								~(marcel_topo_levels[l+1][j+k].vpset)))
								marcel_topo_levels[l+1][j+k].arity++;
						mdebug("fake level %u,%u: vpset %lx arity %u\n",l+1,j+k,marcel_topo_levels[l+1][j+k].vpset,marcel_topo_levels[l+1][j+k].arity);
					}
					marcel_topo_levels[l][i].arity = nbsublevels;
					mdebug("now level %u,%u: vpset %lx has arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
					MA_BUG_ON(k!=nbsublevels);
					j += nbsublevels;
				}
				MA_BUG_ON(j!=levelsize);
				marcel_vpmask_empty(&marcel_topo_levels[l+1][j].vpset);
				if (++marcel_topo_nblevels ==
					sizeof(marcel_topo_levels)/sizeof(*marcel_topo_levels))
					MA_BUG();
			}
		}
#endif

	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {
			if (marcel_topo_levels[l][i].arity) {
				mdebug("level %u,%u: vpset %lx arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
				MA_BUG_ON(!(marcel_topo_levels[l][i].sons=
					TBX_MALLOC(marcel_topo_levels[l][i].arity*sizeof(void *))));

				m=0;
				for (j=0; marcel_topo_levels[l+1][j].vpset; j++)
					if (!(marcel_topo_levels[l+1][j].vpset &
						~(marcel_topo_levels[l][i].vpset))) {
						marcel_topo_levels[l][i].sons[m]=
							&marcel_topo_levels[l+1][j];
						marcel_topo_levels[l+1][j].father = &marcel_topo_levels[l][i];
						marcel_topo_levels[l+1][j].index = m++;
					}
			}
		}
	}
}

void ma_topo_exit(void) {
	unsigned l,i;
	for (l=0; l<marcel_topo_nblevels; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {
			TBX_FREE(marcel_topo_levels[l][i].sons);
		}
		if (l)
			TBX_FREE(marcel_topo_levels[l]);
	}
}

__ma_initfunc(topo_discover, MA_INIT_TOPOLOGY, "Finding Topology");

#ifdef MA__NUMA
static void topology_lwp_init(ma_lwp_t lwp) {
	int i;
	if (marcel_topo_node_level) {
		for (i=0; marcel_topo_node_level[i].vpset; i++) {
			if (marcel_vpmask_vp_ismember(&marcel_topo_node_level[i].vpset,LWP_NUMBER(lwp))) {
				ma_per_lwp(node_level,lwp) = &marcel_topo_node_level[i];
				break;
			}
		}
	}
#ifdef MARCEL_SMT_IDLE
	if (marcel_topo_core_level) {
		for (i=0; marcel_topo_core_level[i].vpset; i++) {
			if (marcel_vpmask_vp_ismember(&marcel_topo_core_level[i].vpset,LWP_NUMBER(lwp))) {
				ma_per_lwp(core_level,lwp) = &marcel_topo_core_level[i];
				break;
			}
		}
	}
#endif /* MARCEL_SMT_IDLE */
	if (marcel_topo_cpu_level) {
		for (i=0; marcel_topo_cpu_level[i].vpset; i++) {
			if (marcel_vpmask_vp_ismember(&marcel_topo_cpu_level[i].vpset,LWP_NUMBER(lwp))) {
				ma_per_lwp(cpu_level,lwp) = &marcel_topo_cpu_level[i];
				break;
			}
		}
	}
}

static void topology_lwp_start(ma_lwp_t lwp) {
}

MA_DEFINE_LWP_NOTIFIER_START(topology, "Topology",
				  topology_lwp_init, "Initialisation de la topologie",
				  topology_lwp_start, "Activation de la topologie");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(topology, MA_INIT_TOPOLOGY);
#endif /* MA__NUMA */

#endif /* MA__LWPS */
