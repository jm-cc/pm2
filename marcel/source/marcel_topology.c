
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

struct marcel_topo_level marcel_machine_level[1+MARCEL_NBMAXVPSUP+1] = {
	{
		.type = MARCEL_LEVEL_MACHINE,
		.number = 0,
		.index = 0,
		.vpset = MARCEL_VPMASK_FULL,
		.arity = 0,
		.children = NULL,
		.father = NULL,
#ifdef MARCEL_SMT_IDLE
		.nbidle = MA_ATOMIC_INIT(0),
#endif
#ifdef MA__SMP
		.kmutex = MARCEL_KTHREAD_MUTEX_INITIALIZER,
		.kneed = MARCEL_KTHREAD_COND_INITIALIZER,
		.kneeddone = MARCEL_KTHREAD_COND_INITIALIZER,
		.spare = 0,
		.needed = -1,
#endif
		.leveldata.vpdata = MARCEL_TOPO_VPDATA_INITIALIZER(&marcel_machine_level[0].leveldata.vpdata),
	},
	{
		.vpset = MARCEL_VPMASK_EMPTY,
	}
};

#ifdef MA__NUMA
static marcel_vpmask_t vpmask;
#endif

int ma_vp_node[MA_NR_LWPS];

#undef marcel_topo_vp_level
struct marcel_topo_level *marcel_topo_vp_level = marcel_machine_level;

unsigned marcel_topo_level_nbitems[2*MARCEL_LEVEL_LAST+1] = { 1 };
struct marcel_topo_level *marcel_topo_levels[2*MARCEL_LEVEL_LAST+1] = {
	marcel_machine_level
};

#undef marcel_current_vp
unsigned marcel_current_vp(void)
{
	return __marcel_current_vp();
}

marcel_topo_level_t *marcel_topo_level(unsigned level, unsigned index) {
	if (index > marcel_topo_level_nbitems[level])
		return NULL;
	return &marcel_topo_levels[level][index];
}

#ifdef MA__LWPS

static int discovering_level = 1;
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 1;
unsigned marcel_vps_per_cpu = 1;
#ifdef MA__NUMA
unsigned marcel_topo_max_arity = 0;
#endif

void ma_set_nbprocessors(void) {
	// Détermination du nombre de processeurs disponibles
	marcel_nbprocessors = ma_nbprocessors();
	mdebug("%d processors available\n", marcel_nbprocessors);
}

void ma_set_processors(void) {
	marcel_vps_per_cpu = (marcel_nbvps()+marcel_nbprocessors-1)/marcel_nbprocessors;
	if (marcel_vps_per_cpu == 1)
		/* no more vps than cpus, distribute them */
		marcel_cpu_stride = marcel_nbprocessors / marcel_nbvps();
	mdebug("%d LWP%s per cpu, stride %d\n", marcel_vps_per_cpu, marcel_vps_per_cpu == 1 ? "" : "s", marcel_cpu_stride);
}

#ifdef MA__NUMA

#ifdef MARCEL_SMT_IDLE
static struct marcel_topo_level *marcel_topo_core_level;
#endif
#undef marcel_topo_node_level
struct marcel_topo_level *marcel_topo_node_level;
static struct marcel_topo_level *marcel_topo_cpu_level;

#ifdef LINUX_SYS
#define PROCESSOR	"processor"
#define PHYSID		"physical id"
#define COREID		"core id"
//#define THREADID	"thread id"

static void __marcel_init look_cpuinfo(void) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	long processor=-1, physid, maxphysid=-1, coreid, maxcoreid=-1;
	int i,j,k;
	unsigned proc_physid[MARCEL_NBMAXCPUS],proc_coreid[MARCEL_NBMAXCPUS];

	unsigned cpu, cpu2;

	unsigned numdies=0;
	int really_dies=0;

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
			char *c = strchr(string, ':')+1; \
			var = strtol(c,&endptr,0); \
			if (endptr==c) { \
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
		getprocnb_begin(PHYSID,physid);
			proc_physid[processor]=physid;
			if (physid>maxphysid)
				maxphysid=physid;
		getprocnb_end() else
		getprocnb_begin(COREID,coreid);
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

	int dienum[maxphysid+1];
	ma_cpu_set_t diecpus[maxphysid+1];
	struct marcel_topo_level *die_level;

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
		MA_BUG_ON(!(die_level=TBX_MALLOC((numdies+MARCEL_NBMAXVPSUP+1)*sizeof(*die_level))));

		for (cpu=0, j=0; cpu <= processor; cpu++) {
			i = proc_physid[cpu];
			if (dienum[i] < 0) {
				dienum[i] = -dienum[i]-1;
				die_level[j].type = MARCEL_LEVEL_DIE;
				die_level[j].number = j;
				marcel_vpmask_empty(&die_level[j].vpset);
				for (k=0; k < marcel_nbvps(); k++) {
					cpu2 = ma_cpu_of_vp_num(k);
					if (MA_CPU_ISSET(cpu2,&diecpus[i]))
						marcel_vpmask_add_vp(&die_level[j].vpset,k);
				}
				die_level[i].arity=0;
				die_level[i].children=NULL;
				die_level[i].father=NULL;
				mdebug("die %d has vpset %"MA_PRIxVPM"\n",j,die_level[j].vpset);
				j++;
			}
		}

		marcel_vpmask_empty(&die_level[j].vpset);

		marcel_topo_level_nbitems[discovering_level]=numdies;
		marcel_topo_levels[discovering_level++]=die_level;
	}

	int corenum[numdies*(maxcoreid+1)];
	ma_cpu_set_t corecpus[numdies*(maxcoreid+1)];
	struct marcel_topo_level *core_level;

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
		MA_BUG_ON(!(core_level=TBX_MALLOC((numcores+MARCEL_NBMAXVPSUP+1)*sizeof(*core_level))));

		for (cpu=0, j=0; cpu <= processor; cpu++) {
			physid = proc_physid[cpu];
			i = dienum[physid]+proc_coreid[cpu]*numdies;
			if (corenum[i] < 0) {
				corenum[i] = -corenum[i]-1;
				core_level[j].type = MARCEL_LEVEL_CORE;
				core_level[j].number = j;
				marcel_vpmask_empty(&core_level[j].vpset);
				for (k=0; k < marcel_nbvps(); k++) {
					cpu2 = ma_cpu_of_vp_num(k);
					if (MA_CPU_ISSET(cpu2,&corecpus[i]))
						marcel_vpmask_add_vp(&core_level[j].vpset,k);
				}
				core_level[i].arity=0;
				core_level[i].children=NULL;
				core_level[i].father=NULL;
#ifdef MARCEL_SMT_IDLE
				ma_atomic_set(&core_level[j].nbidle, 0);
#endif
				mdebug("core %d has vpset %"MA_PRIxVPM"\n",j,core_level[j].vpset);
				j++;
			}
		}

		marcel_vpmask_empty(&core_level[j].vpset);

		marcel_topo_level_nbitems[discovering_level]=numcores;
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

	MA_BUG_ON(!(node_level=TBX_MALLOC((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*node_level))));

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
		node_level[i].vpset=vpset=buffer[0]&vpmask;
		mdebug("node %d has vpset %"MA_PRIxVPM"\n",i,vpset);
		for (j=0;j<marcel_nbvps();j++)
			if (marcel_vpmask_vp_ismember(&vpset,j))
				ma_vp_node[j]=i;
		node_level[i].arity=0;
		node_level[i].children=NULL;
		node_level[i].father=NULL;
	}

	marcel_vpmask_empty(&node_level[i].vpset);

	marcel_topo_level_nbitems[discovering_level]=nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;

	TBX_FREE(buffer);
}
#endif


#ifdef OSF_SYS
#include <numa.h>
static void __marcel_init look_libnuma(void) {
	cpu_cursor_t cursor;
	unsigned i;
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

	MA_BUG_ON(!(node_level=TBX_MALLOC((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*node_level))));

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
			if (cpuid < marcel_nbvps()) {
				marcel_vpmask_add_vp(&node_level[radid].vpset,cpuid);
				ma_vp_node[cpuid]=radid;
			}
		mdebug("node %d has vpset %"MA_PRIxVPM"\n",i,node_level[radid].vpset);
		node_level[radid].arity=0;
		node_level[radid].children=NULL;
		node_level[radid].father=NULL;
	}

	marcel_vpmask_empty(&node_level[i].vpset);

	marcel_topo_level_nbitems[discovering_level]=nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;
}
#endif

#ifdef AIX_SYS
#include <sys/rset.h>
static void __marcel_init look_rset(int sdl, enum marcel_topo_level_e level) {
	rsethandle_t rset, rad;
	int r,i,nbcpus,j;
	unsigned nbnodes;
	struct marcel_topo_level *rad_level;

	rset = rs_alloc(RS_PARTITION);
	rad = rs_alloc(RS_EMPTY);
	nbnodes = rs_numrads(rset, sdl, 0);
	if (nbnodes == -1) {
		perror("rs_numrads");
		nbnodes = 0;
		return;
	}
	if (nbnodes == 1) {
		nbnodes = 0;
		return;
	}

	MA_BUG_ON(nbnodes == 0);

	MA_BUG_ON(!(rad_level=TBX_MALLOC((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*rad_level))));

	for (r = 0, i = 0; i < nbnodes; i++) {
		if (rs_getrad(rset, rad, sdl, i, 0)) {
			fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
			continue;
		}
		if (!rs_getinfo(rad, R_NUMPROCS, 0))
			continue;

		rad_level[r].type = level;
		rad_level[r].number=r;
		marcel_vpmask_empty(&rad_level[r].vpset);
		nbcpus = rs_getinfo(rad, R_NUMPROCS, 0);
		for (j = 0; j < marcel_nbvps(); j++) {
			if (!rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
				continue;
			marcel_vpmask_add_vp(&rad_level[r].vpset,j);
			if (level == MARCEL_LEVEL_NODE)
				ma_vp_node[j]=r;
		}
		mdebug("node %d has vpset %"MA_PRIxVPM"\n",r,rad_level[r].vpset);
		rad_level[r].arity=0;
		rad_level[r].children=NULL;
		rad_level[r].father=NULL;
		r++;
	}

	marcel_vpmask_empty(&rad_level[r].vpset);

	marcel_topo_level_nbitems[discovering_level]=nbnodes;
	marcel_topo_levels[discovering_level++] = rad_level;
	if (level == MARCEL_LEVEL_NODE)
		marcel_topo_node_level = rad_level;
#ifdef MARCEL_SMT_IDLE
	if (level == MARCEL_LEVEL_CORE)
		marcel_topo_core_level = rad_level;
#endif
	rs_free(rset);
	rs_free(rad);
}
#endif


static void look_cpu(void) {
	struct marcel_topo_level *cpu_level;
	unsigned i,cpu;

	if (marcel_nbprocessors>=marcel_nbvps())
		return;

	MA_BUG_ON(!(cpu_level=TBX_MALLOC((marcel_nbprocessors+MARCEL_NBMAXVPSUP+1)*sizeof(*cpu_level))));

	for (cpu=0; cpu<marcel_nbprocessors; cpu++) {
		cpu_level[cpu].type=MARCEL_LEVEL_PROC;
		cpu_level[cpu].number=cpu;
		marcel_vpmask_empty(&cpu_level[cpu].vpset);
		cpu_level[cpu].arity=0;
		cpu_level[cpu].children=NULL;
		cpu_level[cpu].father=NULL;
	}
	marcel_vpmask_empty(&cpu_level[cpu].vpset);

	for (i=0; i<marcel_nbvps(); i++) {
		cpu = ma_cpu_of_vp_num(i);
		marcel_vpmask_add_vp(&cpu_level[cpu].vpset,i);
	}

	for (cpu=0; cpu<marcel_nbprocessors; cpu++)
		mdebug("cpu %d has vpset %"MA_PRIxVPM"\n",cpu,cpu_level[cpu].vpset);

	marcel_topo_level_nbitems[discovering_level]=marcel_nbprocessors;
	marcel_topo_levels[discovering_level++]=
		marcel_topo_cpu_level = cpu_level;
}
#endif /* MA__NUMA */

static void look_vp(void) {
	struct marcel_topo_level *vp_level;
	unsigned i;

	if (marcel_nbvps()==1)
		return;

	MA_BUG_ON(!(vp_level=TBX_MALLOC((marcel_nbvps()+MARCEL_NBMAXVPSUP+1)*sizeof(*vp_level))));

	/* do not initialize supplementary VPs yet, since they may end up on the machine level on a single-CPU machine */
	for (i=0; i<marcel_nbvps(); i++) {
		vp_level[i].type=MARCEL_LEVEL_VP;
		vp_level[i].number=i;
		marcel_vpmask_only_vp(&vp_level[i].vpset,i);
		vp_level[i].arity=0;
		vp_level[i].children=NULL;
		vp_level[i].father=NULL;
	}
	marcel_vpmask_empty(&vp_level[i].vpset);

	marcel_topo_level_nbitems[discovering_level]=marcel_nbvps();
	marcel_topo_levels[discovering_level++] = vp_level;
}

#ifdef MA__NUMA
/* split arity into nbsublevels of size sublevelarity */
static void split(unsigned arity, unsigned * __restrict sublevelarity, unsigned * __restrict nbsublevels) {
	*sublevelarity = sqrt(arity);
	*nbsublevels = (arity + *sublevelarity-1) / *sublevelarity;
	if (*nbsublevels > marcel_topo_max_arity) {
		*nbsublevels = marcel_topo_max_arity;
		*sublevelarity = (arity + *nbsublevels-1) / *nbsublevels;
	}
}
#endif

static void topo_discover(void) {
	unsigned l,i,j,m,n;
	struct marcel_topo_level *level;
#ifdef MA__NUMA
	unsigned k;
	unsigned nbsublevels;
	unsigned sublevelarity;
	unsigned vp;
	int dosplit;

	for (i = 0; i < marcel_nbvps() + MARCEL_NBMAXVPSUP; i++)
		ma_vp_node[i]=-1;

	marcel_vpmask_empty(&vpmask);
	for (vp=0; vp < marcel_nbvps(); vp++)
		marcel_vpmask_add_vp(&vpmask, vp);
	memcpy(&marcel_machine_level[0].vpset, &vpmask, sizeof(vpmask));
#ifdef LINUX_SYS
	look_libnuma();
	look_cpuinfo();
#endif
#ifdef OSF_SYS
	look_libnuma();
#endif
#ifdef  AIX_SYS
	enum marcel_topo_level_e mlevel = MARCEL_LEVEL_MACHINE;

	for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++) {
		if (i == rs_getinfo(NULL, R_MCMSDL, 0))
			mlevel = MARCEL_LEVEL_NODE;
#ifdef R_L2CSDL
		else if (i == rs_getinfo(NULL, R_L2CSDL, 0))
			mlevel = MARCEL_LEVEL_DIE;
#endif
#ifdef R_PCORESDL
		else if (i == rs_getinfo(NULL, R_PCORESDL, 0))
			mlevel = MARCEL_LEVEL_CORE;
#endif
		else if (i == rs_getinfo(NULL, R_MAXSDL, 0))
			mlevel = MARCEL_LEVEL_PROC;
		mdebug("looking AIX sdl %d\n",i);
		look_rset(i, mlevel);
	}
#endif
	look_cpu();
#endif
	look_vp();

	marcel_topo_nblevels=discovering_level;

	l = marcel_topo_nblevels-1;
	for (i=0; marcel_topo_levels[l][i].vpset; i++)
		marcel_topo_levels[l][i].type = MARCEL_LEVEL_VP;

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
			memmove(&marcel_topo_level_nbitems[l+1],&marcel_topo_level_nbitems[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_level_nbitems));
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
			mdebug("level %u,%u: vpset %"MA_PRIxVPM" arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
		}
	}
	mdebug("arity done.\n");

#ifdef MA__NUMA
	/* Split hi-arity levels */
	if (marcel_topo_max_arity) {
		/* connect levels */
		for (l=0; l<marcel_topo_nblevels-1; l++) {
			for (i=0; marcel_topo_levels[l][i].vpset; i++) {
				if (marcel_topo_levels[l][i].arity) {
					mdebug("level %u,%u: vpset %"MA_PRIxVPM" arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
					MA_BUG_ON(!(marcel_topo_levels[l][i].children=
						TBX_MALLOC(marcel_topo_levels[l][i].arity*sizeof(void *))));

					m=0;
					for (j=0; marcel_topo_levels[l+1][j].vpset; j++)
						if (!(marcel_topo_levels[l+1][j].vpset &
							~(marcel_topo_levels[l][i].vpset))) {
							marcel_topo_levels[l][i].children[m]=
								&marcel_topo_levels[l+1][j];
							marcel_topo_levels[l+1][j].father = &marcel_topo_levels[l][i];
							marcel_topo_levels[l+1][j].index = m++;
						}
				}
			}
		}
		mdebug("pre-connecting done.\n");

		/* For each level */
		for (l=0; l<marcel_topo_nblevels-1; l++) {
			unsigned level_width = 0;
			dosplit = 0;
			/* Look at each item, check for max_arity */
			for (i=0; marcel_topo_levels[l][i].vpset; i++) {
				split(marcel_topo_levels[l][i].arity,&sublevelarity,&nbsublevels);
				if (marcel_topo_levels[l][i].arity > marcel_topo_max_arity) {
					mdebug("will split level %u,%u because %d > %d\n", l, i, marcel_topo_levels[l][i].arity, marcel_topo_max_arity);
					dosplit=1;
				}
				level_width += nbsublevels;
			}
			if (dosplit) {
				/* split needed, shift levels */
				memmove(&marcel_topo_level_nbitems[l+2],&marcel_topo_level_nbitems[l+1],(marcel_topo_nblevels-(l+1))*sizeof(*marcel_topo_level_nbitems));
				memmove(&marcel_topo_levels[l+2],&marcel_topo_levels[l+1],(marcel_topo_nblevels-(l+1))*sizeof(*marcel_topo_levels));
				/* new fake level */
				marcel_topo_level_nbitems[l+1] = level_width;
				marcel_topo_levels[l+1] = TBX_MALLOC((level_width+MARCEL_NBMAXVPSUP+1)*sizeof(**marcel_topo_levels));
				/* fill it with split items */
				for (i=0,j=0; marcel_topo_levels[l][i].vpset; i++) {
					/* split one item */
					split(marcel_topo_levels[l][i].arity,&sublevelarity,&nbsublevels);
					mdebug("splitting level %u,%u into %u sublevels of size at most %u\n", l, i, nbsublevels, sublevelarity);
					/* initialize subitems */
					for (k=0; k<nbsublevels; k++) {
						level = &marcel_topo_levels[l+1][j+k];
						level->type = MARCEL_LEVEL_FAKE;
						level->number = j+k;
						marcel_vpmask_empty(&level->vpset);
						marcel_topo_levels[l+1][j+k].arity=0;
					}

					/* distribute cpus to subitems */
					/* give cpus of sublevelarity items to each fake item */

					/* first fake item */
					k = 0;
					/* will get first item's cpus */
					m = 0;
					for (n=0; n<marcel_topo_levels[l][i].arity; n++) {
						marcel_vpmask_or(&marcel_topo_levels[l+1][j+k].vpset,
							&marcel_topo_levels[l][i].children[n]->vpset);
						marcel_topo_levels[l+1][j+k].arity++;
						if (++m == sublevelarity) {
							k++;
							m = 0;
						}
					}
					marcel_topo_levels[l][i].arity = nbsublevels;
					mdebug("now level %u,%u: vpset %"MA_PRIxVPM" has arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
					MA_BUG_ON(k!=nbsublevels);
					j += nbsublevels;
				}
				MA_BUG_ON(j!=level_width);
				marcel_vpmask_empty(&marcel_topo_levels[l+1][j].vpset);
				if (++marcel_topo_nblevels ==
					sizeof(marcel_topo_levels)/sizeof(*marcel_topo_levels))
					MA_BUG();
			}
		}
	}
#endif

	/* and finally connect levels */
	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {
			if (marcel_topo_levels[l][i].arity) {
				mdebug("level %u,%u: vpset %"MA_PRIxVPM" arity %u\n",l,i,marcel_topo_levels[l][i].vpset,marcel_topo_levels[l][i].arity);
				MA_BUG_ON(!(marcel_topo_levels[l][i].children=
					TBX_MALLOC(marcel_topo_levels[l][i].arity*sizeof(void *))));

				m=0;
				for (j=0; marcel_topo_levels[l+1][j].vpset; j++)
					if (!(marcel_topo_levels[l+1][j].vpset &
						~(marcel_topo_levels[l][i].vpset))) {
						marcel_topo_levels[l][i].children[m]=
							&marcel_topo_levels[l+1][j];
						marcel_topo_levels[l+1][j].father = &marcel_topo_levels[l][i];
						marcel_topo_levels[l+1][j].index = m++;
					}
			}
		}
	}
	mdebug("connecting done.\n");
	marcel_topo_vp_level = marcel_topo_levels[marcel_topo_nblevels-1];
	for (i=marcel_nbvps(); i<marcel_nbvps() + MARCEL_NBMAXVPSUP; i++) {
		marcel_topo_vp_level[i].type=MARCEL_LEVEL_VP;
		marcel_topo_vp_level[i].number=i;
		marcel_vpmask_only_vp(&marcel_topo_vp_level[i].vpset,i);
		marcel_topo_vp_level[i].arity=0;
		marcel_topo_vp_level[i].children=NULL;
		marcel_topo_vp_level[i].father=NULL;
	}
	marcel_vpmask_empty(&marcel_topo_vp_level[i].vpset);

	for (level = &marcel_topo_vp_level[0]; level < &marcel_topo_vp_level[marcel_nbvps() + MARCEL_NBMAXVPSUP]; level++)
		level->leveldata.vpdata = (struct marcel_topo_vpdata) MARCEL_TOPO_VPDATA_INITIALIZER(&level->leveldata.vpdata);
}

void ma_topo_exit(void) {
	unsigned l,i;
	/* Last level is not freed because we need it in various places */
	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].vpset; i++) {
			TBX_FREE(marcel_topo_levels[l][i].children);
			marcel_topo_levels[l][i].children = NULL;
		}
		if (l) {
			TBX_FREE(marcel_topo_levels[l]);
			marcel_topo_levels[l] = NULL;
		}
	}
}

__ma_initfunc(topo_discover, MA_INIT_TOPOLOGY, "Finding Topology");

static void topology_lwp_init(ma_lwp_t lwp) {
#ifdef MA__NUMA
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
#endif /* MA__NUMA */
	ma_per_lwp(vp_level, lwp) = &marcel_topo_vp_level[LWP_NUMBER(lwp)];
}

static void topology_lwp_start(ma_lwp_t lwp) {
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(topology, 400, "Topology",
				  topology_lwp_init, "Initialisation de la topologie",
				  topology_lwp_start, "Activation de la topologie");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(topology, MA_INIT_TOPOLOGY);

#endif /* MA__LWPS */
