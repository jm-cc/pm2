
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
		.cpuset = MA_CPU_FULL,
		.arity = 0,
		.sons = NULL,
#ifdef MARCEL_SMT_IDLE
		.nbidle = MA_ATOMIC_INIT(0),
#endif
	},
	{
		.cpuset = MA_CPU_EMPTY,
	}
};


struct marcel_topo_level *marcel_topo_levels[MARCEL_LEVEL_LAST+1] = {
	marcel_machine_level
};

#ifdef MA__LWPS

static int discovering_level=1;

#ifdef MA__NUMA

#ifdef MARCEL_SMT_IDLE
MA_DEFINE_PER_LWP(struct marcel_topo_level *, core_level, NULL);
static struct marcel_topo_level *marcel_topo_core_level;
#endif

#ifdef LINUX_SYS
#define PROCESSOR	"processor\t: "
#define PHYSID		"physical id\t: "

static void __marcel_init look_cpuinfo(void) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	long processor=-1, physid, maxphysid=-1;
	int i,j;
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
		ma_cpu_set_t corecpuset[maxphysid+1];
		struct marcel_topo_level *core_level;
		unsigned numcores=0;
		int really_useful=0;

		memset(corenum,0,sizeof(corenum));
		memset(corecpuset,0,sizeof(corecpuset));

		for (i=0; i<= processor; i++) {
			if (!corenum[proc_physid[i]]) {
				corenum[proc_physid[i]] = 1;
				numcores++;
			} else really_useful=1;
			MA_CPU_SET(i,&corecpuset[proc_physid[i]]);
		}

		if (!really_useful)
			return;

		MA_BUG_ON(!(core_level=TBX_MALLOC((numcores+1)*sizeof(*core_level))));

		for (i=0, j=0; i<= maxphysid; i++) {
			if (corenum[i]) {
				core_level[j].type = MARCEL_LEVEL_CORE;
				core_level[j].number = j;
				core_level[j].cpuset = corecpuset[i];
				mdebug("core %d has cpuset %lx\n",j,corecpuset[i]);
				j++;
			}
		}

		MA_CPU_ZERO(&core_level[j].cpuset);

#ifdef MARCEL_SMT_IDLE
		marcel_topo_core_level =
#endif
		marcel_topo_levels[discovering_level++]=core_level;
	}
}
#endif

#include <numa.h>
static void __marcel_init look_libnuma(void) {
	unsigned long *buffer,*buffer2;
	unsigned buffersize=MARCEL_NBMAXCPUS/8;
	unsigned i;
	unsigned nbnodes;
	struct marcel_topo_level *node_level;

	numa_exit_on_error=1;

	if (numa_available()==-1)
		return;

	nbnodes=numa_max_node()+1;
	if (nbnodes==1) {
		nbnodes=0;
		return;
	}

	MA_BUG_ON(nbnodes==0);

	MA_BUG_ON(!(node_level=TBX_MALLOC((nbnodes+1)*sizeof(*node_level))));

	if (!(buffer=TBX_MALLOC(buffersize))) {
		fprintf(stderr,"no room for storing cpu mask\n");
		return;
	}

	while (numa_node_to_cpus(0, buffer, buffersize)==-1 && errno==ERANGE) {
		buffersize*=2;
		if (!(buffer2 = TBX_REALLOC(buffer, buffersize))) {
			fprintf(stderr,"no room for storing cpu mask\n");
			free(buffer);
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
		node_level[i].cpuset=buffer[0];
		mdebug("node %d has cpuset %lx\n",i,buffer[0]);
	}

	MA_CPU_ZERO(&node_level[i].cpuset);

	marcel_topo_levels[discovering_level++]=node_level;
}
#endif /* MA__NUMA */

static void look_cpu(void) {
	unsigned __nb_processors;
	struct marcel_topo_level *cpu_level;
	unsigned i;

	// Détermination du nombre de processeurs disponibles
#ifdef SOLARIS_SYS
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(LINUX_SYS)
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
  //__nb_processors = 2;
#elif defined(IRIX_SYS)
	__nb_processors = sysconf(_SC_NPROC_CONF);
#elif defined(OSF_SYS)
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
#else
#warning __nb_processors set to 1 for this system
	__nb_processors = 1;
#endif

	mdebug("%d processors available\n", __nb_processors);

	if (__nb_processors==1)
		return;

	MA_BUG_ON(!(cpu_level=TBX_MALLOC((__nb_processors+1)*sizeof(*cpu_level))));

	for (i=0; i<__nb_processors; i++) {
		cpu_level[i].type=MARCEL_LEVEL_PROC;
		cpu_level[i].number=i;
		MA_CPU_ZERO(&cpu_level[i].cpuset);
		MA_CPU_SET(i,&cpu_level[i].cpuset);
		cpu_level[i].arity=0;
		cpu_level[i].sons=NULL;
	}
	MA_CPU_ZERO(&cpu_level[i].cpuset);

	marcel_topo_levels[discovering_level++]=cpu_level;
}

static void topo_discover(void) {
	unsigned l,i,j,m;
#ifdef MA__NUMA
	look_libnuma();
#ifdef LINUX_SYS
	look_cpuinfo();
#endif
#endif
	look_cpu();

	marcel_topo_nblevels=discovering_level;

	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].cpuset; i++) {

			marcel_topo_levels[l][i].arity=0;
			for (j=0; marcel_topo_levels[l+1][j].cpuset; j++)
				if (!(marcel_topo_levels[l+1][j].cpuset &
					~(marcel_topo_levels[l][i].cpuset)))
					marcel_topo_levels[l][i].arity++;

			mdebug("level %u,%u: cpuset %lx arity %u\n",l,i,marcel_topo_levels[l][i].cpuset,marcel_topo_levels[l][i].arity);
			MA_BUG_ON(!(marcel_topo_levels[l][i].sons=
				TBX_MALLOC(marcel_topo_levels[l][i].arity*sizeof(void *))));

			m=0;
			for (j=0; marcel_topo_levels[l+1][j].cpuset; j++)
				if (!(marcel_topo_levels[l+1][j].cpuset &
					~(marcel_topo_levels[l][i].cpuset)))
					marcel_topo_levels[l][i].sons[m++]=
						&marcel_topo_levels[l+1][j];
		}
	}
}

__ma_initfunc(topo_discover, MA_INIT_TOPOLOGY, "Finding Topology");

#ifdef MARCEL_SMT_IDLE
static void topology_lwp_init(ma_lwp_t lwp) {
	if (marcel_topo_core_level) {
		int i;
		for (i=0; marcel_topo_core_level[i].cpuset; i++) {
			if (MA_CPU_ISSET(LWP_NUMBER(lwp),&marcel_topo_core_level[i].cpuset)) {
				ma_per_lwp(core_level,lwp) = &marcel_topo_core_level[i];
				break;
			}
		}
	}
}

static void topology_lwp_start(ma_lwp_t lwp) {
}

MA_DEFINE_LWP_NOTIFIER_START(topology, "Topology",
				  topology_lwp_init, "Initialisation de la topologie",
				  topology_lwp_start, "Activation de la topologi");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(topology, MA_INIT_TOPOLOGY);
#endif

#endif /* MA__LWPS */
