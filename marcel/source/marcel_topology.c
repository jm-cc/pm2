
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


struct marcel_topo_level *marcel_topo_levels[MARCEL_LEVEL_LAST+1] = {
	marcel_machine_level
};

#ifdef MA__LWPS

static int discovering_level = 1;
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 1;
unsigned marcel_lwps_per_cpu = 1;

void ma_set_nbprocessors(void) {
	// Détermination du nombre de processeurs disponibles
#if defined(SOLARIS_SYS) || defined(LINUX_SYS) || defined(OSF_SYS) || defined(WIN_SYS)
	marcel_nbprocessors = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(IRIX_SYS)
	marcel_nbprocessors = sysconf(_SC_NPROC_CONF);
#else
#warning No known way to discover number of available processors on this system
#warning marcel_nbprocessors will default to 1
#warning use the --marcel-nvp option to set it by hand when running your program
	marcel_nbprocessors = 1;
#endif

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
#define PHYSID		"physical id\t: "

static void __marcel_init look_cpuinfo(void) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	long processor=-1, physid, maxphysid=-1;
	int i,j,k;
	unsigned proc_physid[MARCEL_NBMAXCPUS];

	if (!(fd=fopen("/proc/cpuinfo","r"))) {
		fprintf(stderr,"could not open /proc/cpuinfo\n");
		return;
	}

	while (fgets(string,sizeof(string),fd)!=NULL) {
		if (!strncmp(PROCESSOR,string,strlen(PROCESSOR))) {
			processor = strtol(string+strlen(PROCESSOR),&endptr,0);
			if (endptr==string+strlen(PROCESSOR)) {
				fprintf(stderr,"no number in processor field of /proc/cpuinfo\n");
				break;
			} else if (processor<0) {
				fprintf(stderr,"too small processor number in /proc/cpuinfo\n");
				break;
			} else if (processor>=MARCEL_NBMAXCPUS) {
				fprintf(stderr,"too big processor number in /proc/cpuinfo\n");
				break;
			}
		}
		if (!strncmp(PHYSID,string,strlen(PHYSID))) {
			physid = strtol(string+strlen(PHYSID),&endptr,0);
			if (endptr==string+strlen(PHYSID)) {
				fprintf(stderr,"no number in physical id field of /proc/cpuinfo\n");
				break;
			} else if (physid==LONG_MIN) {
				fprintf(stderr,"too small physical id %ld in /proc/cpuinfo\n",physid);
				break;
			} else if (physid==LONG_MAX) {
				fprintf(stderr,"too big physical id %ld in /proc/cpuinfo\n",physid);
				break;
			}
			proc_physid[processor]=physid;
			if (physid>maxphysid)
				maxphysid=physid;
		}
		if (string[strlen(string)-1]!='\n') {
			fscanf(fd,"%*[^\n]");
			getc(fd);
		}
	}
	fclose(fd);

	if (maxphysid>=0) {
		/* normalize core numbers */
		unsigned corenum[maxphysid+1];
		ma_cpu_set_t corecpus[maxphysid+1];
		struct marcel_topo_level *core_level;
		unsigned numcores=0;
		int really_useful=0;
		ma_cpu_set_t cpuset;
		unsigned cpu;

		memset(corenum,0,sizeof(corenum));
		memset(corecpus,0,sizeof(corecpus));

		MA_CPU_ZERO(&cpuset);

		for (cpu=0; cpu <= processor; cpu++) {
			if (!corenum[proc_physid[cpu]]) {
				corenum[proc_physid[cpu]] = 1;
				numcores++;
			} else really_useful=1;
			MA_CPU_SET(cpu,&corecpus[proc_physid[cpu]]);
		}

		if (!really_useful)
			return;

		MA_BUG_ON(!(core_level=TBX_MALLOC((numcores+1)*sizeof(*core_level))));

		for (i=0, j=0; i<= maxphysid; i++) {
			if (corenum[i]) {
				core_level[j].type = MARCEL_LEVEL_CORE;
				core_level[j].number = j;
				marcel_vpmask_empty(&core_level[j].vpset);
				for (k=0; k <= get_nb_lwps(); k++) {
					cpu = ma_cpu_of_lwp_num(k);
					if (MA_CPU_ISSET(cpu,&corecpus[i]))
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
static int numa_not_available;
static void __marcel_init look_libnuma(void) {
	unsigned long *buffer,*buffer2;
	unsigned buffersize=MARCEL_NBMAXCPUS/8;
	unsigned i,j;
	unsigned nbnodes;
	struct marcel_topo_level *node_level;
	marcel_vpmask_t vpset;

	numa_exit_on_error=1;

	if ((numa_not_available = (numa_available())==-1))
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

	numa_exit_on_error=1;

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
				ma_lwp_node[j]=i;
	}

	marcel_vpmask_empty(&node_level[i].vpset);

	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;

	TBX_FREE(buffer);
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

static void topo_discover(void) {
	unsigned l,i,j,m;
#ifdef MA__NUMA
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

	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {

			marcel_topo_levels[l][i].arity=0;
			for (j=0; marcel_topo_levels[l+1][j].vpset; j++)
				if (!(marcel_topo_levels[l+1][j].vpset &
					~(marcel_topo_levels[l][i].vpset)))
					marcel_topo_levels[l][i].arity++;

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
	if (marcel_topo_node_level) {
		for (i=0; marcel_topo_node_level[i].vpset; i++) {
			if (marcel_vpmask_vp_ismember(&marcel_topo_node_level[i].vpset,LWP_NUMBER(lwp))) {
				ma_per_lwp(node_level,lwp) = &marcel_topo_node_level[i];
				break;
			}
		}
	}
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

#ifdef MA__NUMA
#ifdef LINUX_SYS
void *ma_malloc_node(unsigned size, int node, char *file, unsigned line) {
	void *p;
	if (node < 0 || numa_not_available)
		return marcel_malloc(size, file, line);
	marcel_extlib_protect();
	p = numa_alloc_onnode(size, node);
	marcel_extlib_unprotect();
	if (p == NULL)
		return marcel_malloc(size, file, line);
	return p;
}
void ma_free_node(void *ptr, unsigned size, int node, char * __restrict file, unsigned line) {
	if (node < 0 || numa_not_available)
		return marcel_free(ptr, file, line);
	marcel_extlib_protect();
	numa_free(ptr, size);
	marcel_extlib_unprotect();
}
#else
#warning "don't know how to allocate memory on specific nodes, please disable numa in flavor"
#endif
#endif
