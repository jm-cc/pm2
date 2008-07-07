
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
#  include <math.h>
#endif

/* cpus */
#ifdef MA__NUMA
#  include <sched.h>
#  ifdef CPU_SET
typedef cpu_set_t ma_cpu_set_t;
#    define MA_CPU_SET(cpu, cpusetp)	CPU_SET(cpu, cpusetp)
#    define MA_CPU_CLR(cpu, cpusetp)	CPU_CLR(cpu, cpusetp)
#    define MA_CPU_ISSET(cpu, cpusetp)	CPU_ISSET(cpu, cpusetp)
#    define MA_CPU_ZERO(cpusetp)		CPU_ZERO(cpusetp)
#  else
typedef unsigned long ma_cpu_set_t;
#    define MA_CPU_MASK(cpu)		(1UL<<(cpu))
#    define MA_CPU_SET(cpu, cpusetp)	(*(cpusetp) |=  MA_CPU_MASK(cpu))
#    define MA_CPU_CLR(cpu, cpusetp)	(*(cpusetp) &= ~MA_CPU_MASK(cpu))
#    define MA_CPU_ISSET(cpu, cpusetp)	(*(cpusetp) &   MA_CPU_MASK(cpu))
#    define MA_CPU_ZERO(cpusetp)		(*(cpusetp) = 0UL)
#  endif
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
		.merged_type = 1<<MARCEL_LEVEL_MACHINE,
		.level = 0,
		.number = 0,
		.index = 0,
		.os_node = -1,
		.os_die = -1,
		.os_l3 = -1,
		.os_l2 = -1,
		.os_core = -1,
		.os_l1 = -1,
		.os_cpu = -1,
		.vpset = MARCEL_VPSET_FULL,
		.cpuset = MARCEL_VPSET_FULL,
		.arity = 0,
		.children = NULL,
		.father = NULL,
#ifdef MARCEL_SMT_IDLE
		.nbidle = MA_ATOMIC_INIT(0),
#endif
#ifdef MA__LWPS
		.kmutex = MARCEL_KTHREAD_MUTEX_INITIALIZER,
		.kneed = MARCEL_KTHREAD_COND_INITIALIZER,
		.kneeddone = MARCEL_KTHREAD_COND_INITIALIZER,
		.spare = 0,
		.needed = -1,
#endif
		.vpdata = MARCEL_TOPO_VPDATA_INITIALIZER(&marcel_machine_level[0].vpdata),
		.nodedata = MARCEL_TOPO_NODEDATA_INITIALIZER(&marcel_machine_level[0].nodedata),
	},
	{
		.vpset = MARCEL_VPSET_ZERO,
		.cpuset = MARCEL_VPSET_ZERO,
	}
};

#ifdef MA__NUMA
int ma_vp_node[MA_NR_LWPS];
#endif

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

const char * marcel_topo_level_string(enum marcel_topo_level_e l)
{
  switch (l) {
  case MARCEL_LEVEL_MACHINE: return "Machine";
#ifdef MA__LWPS
#  ifdef MA__NUMA
  case MARCEL_LEVEL_FAKE: return "Fake";
  case MARCEL_LEVEL_NODE: return "NUMANode";
  case MARCEL_LEVEL_DIE: return "Die";
  case MARCEL_LEVEL_L3: return "L3Cache";
  case MARCEL_LEVEL_L2: return "L2Cache";
  case MARCEL_LEVEL_CORE: return "Core";
  case MARCEL_LEVEL_L1: return "L1Cache";
  case MARCEL_LEVEL_PROC: return "SMTproc";
#  endif
  case MARCEL_LEVEL_VP: return "VP";
#endif
  }
  return "Unknown";
}

void marcel_print_level_description(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode)
{
  unsigned long type = l->merged_type;
  const char * separator = " + ";
  const char * current_separator = ""; /* not prefix for the first one */

  if (!verbose_mode) {
#ifdef MA__LWPS
    /* don't print "vp" if there's something else (including "fake") */
    if (type & ~(1<<MARCEL_LEVEL_VP))
      type &= ~(1<<MARCEL_LEVEL_VP);
#  ifdef MA__NUMA
    /* don't print "fake" if there's something else */
    if (type & ~(1<<MARCEL_LEVEL_FAKE))
      type &= ~(1<<MARCEL_LEVEL_FAKE);
    /* don't print smtproc or caches if there's also core or die */
    if (type & ((1<<MARCEL_LEVEL_CORE) | (1<<MARCEL_LEVEL_DIE)))
      type &= ~( (1<<MARCEL_LEVEL_PROC) | (1<<MARCEL_LEVEL_L1) | (1<<MARCEL_LEVEL_L2) | (1<<MARCEL_LEVEL_L3) );
#  endif
#endif
  }

#define marcel_print_level_description_level(_l) \
  if (type & (1<<_l)) { \
    marcel_fprintf(output, "%s%s", current_separator, marcel_topo_level_string(_l)); \
    current_separator = separator; \
  }

  marcel_print_level_description_level(MARCEL_LEVEL_MACHINE)
#ifdef MA__LWPS
#  ifdef MA__NUMA
  marcel_print_level_description_level(MARCEL_LEVEL_FAKE)
  marcel_print_level_description_level(MARCEL_LEVEL_NODE)
  marcel_print_level_description_level(MARCEL_LEVEL_DIE)
  marcel_print_level_description_level(MARCEL_LEVEL_L3)
  marcel_print_level_description_level(MARCEL_LEVEL_L2)
  marcel_print_level_description_level(MARCEL_LEVEL_CORE)
  marcel_print_level_description_level(MARCEL_LEVEL_L1)
  marcel_print_level_description_level(MARCEL_LEVEL_PROC)
#  endif
  marcel_print_level_description_level(MARCEL_LEVEL_VP)
#endif
}

void marcel_print_level(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode,
			const char *separator, const char *indexprefix, const char* labelseparator, const char* levelterm) {
  marcel_print_level_description(l, output, txt_mode, verbose_mode);
  marcel_fprintf(output, labelseparator);
  if (l->os_node != -1) marcel_fprintf(output, "%sNode %s%u", separator, indexprefix, l->os_node);
  if (l->os_die != -1)  marcel_fprintf(output, "%sDie %s%u" , separator, indexprefix, l->os_die);
  if (l->os_l3 != -1)   marcel_fprintf(output, "%sL3 %s%u"  , separator, indexprefix, l->os_l3);
  if (l->os_l2 != -1)   marcel_fprintf(output, "%sL2 %s%u"  , separator, indexprefix, l->os_l2);
  if (l->os_core != -1) marcel_fprintf(output, "%sCore %s%u", separator, indexprefix, l->os_core);
  if (l->os_l1 != -1)   marcel_fprintf(output, "%sL1 %s%u"  , separator, indexprefix, l->os_l1);
  if (l->os_cpu != -1)  marcel_fprintf(output, "%sCPU %s%u" , separator, indexprefix, l->os_cpu);

  if (l->level == marcel_topo_nblevels-1) {
    marcel_fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  marcel_fprintf(output,"%s\n", levelterm);
}

#ifdef MA__LWPS

static int discovering_level = 1;
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 0;
unsigned marcel_first_cpu = 0;
unsigned marcel_vps_per_cpu = 1;
#  ifdef MA__NUMA
unsigned marcel_nbnodes = 1;
unsigned marcel_topo_max_arity = 4;
#  endif

void ma_set_nbprocessors(void) {
	marcel_nbprocessors = ma_nbprocessors();
	mdebug("%d processors available\n", marcel_nbprocessors);
}

void ma_set_processors(void) {
	marcel_vps_per_cpu = (marcel_nbvps()+marcel_nbprocessors-1)/marcel_nbprocessors;
	if (!marcel_cpu_stride) {
		if (marcel_vps_per_cpu == 1)
			/* no more vps than cpus, distribute them */
			marcel_cpu_stride = marcel_nbprocessors / marcel_nbvps();
		else
			marcel_cpu_stride = 1;
	}
	if (marcel_cpu_stride > 1 && marcel_vps_per_cpu > 1) {
		fprintf(stderr,"More VPs that CPUs doesn't make sense with a stride\n");
		exit(1);
	}
	mdebug("%d LWP%s per cpu, stride %d, from %d\n", marcel_vps_per_cpu, marcel_vps_per_cpu == 1 ? "" : "s", marcel_cpu_stride, marcel_first_cpu);
}

#  ifdef MA__NUMA

static int ma_topo_type_level[MARCEL_LEVEL_LAST + 1];

int ma_get_topo_type_depth (enum marcel_topo_level_e type) {
  return ma_topo_type_level[type];
}

#    ifdef MARCEL_SMT_IDLE
static struct marcel_topo_level *marcel_topo_core_level;
#    endif
#    undef marcel_topo_node_level
struct marcel_topo_level *marcel_topo_node_level;
static struct marcel_topo_level *marcel_topo_cpu_level;

#    ifdef LINUX_SYS
#      define PROCESSOR	"processor"
#      define PHYSID		"physical id"
#      define COREID		"core id"
//#  define THREADID	"thread id"

#define CACHE_LEVEL_MAX 3

static void
ma_parse_shared_cpu_maps(int proc_index, int nr_procs, unsigned *cacheids, unsigned *nr_caches)
{
#define SHARED_CPU_MAP_STRLEN (27+9+12+1+15+1)
	char mappath[SHARED_CPU_MAP_STRLEN];
	char string[20]; /* enough for a shared map mask (32bits hexa), a level (one digit) or a type (Data/Instruction/Unified) */
	FILE * fd;
	int i, k;

	for(i=0; i<10; i++) {
		int level; /* 0 for L1, .... */

		sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/level", proc_index, i);
		fd = fopen(mappath, "r");
		if (fd) {
			if (fgets(string,sizeof(string), fd))
				level = strtol(string, NULL, 10)-1;
			else
				continue;
			fclose(fd);
		} else
			continue;

		sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/type", proc_index, i);
		fd = fopen(mappath, "r");
		if (fd) {
			if (fgets(string,sizeof(string), fd)) {
				if (!strncmp(string,"Instruction", 11))
					continue;
			} else
				continue;
			fclose(fd);
		} else
			continue;

		sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", proc_index, i);
		fd = fopen(mappath, "r");
		if (fd) {
			/* parse the whole mask */
#define MAX_KERNEL_CPU_MASK 8
#define KERNEL_CPU_MASK_BITS 32
			unsigned long maps[MAX_KERNEL_CPU_MASK]; /* support kernels with upto 8*32 processors */
			int nr_maps = 0;
			while (fgets(string, KERNEL_CPU_MASK_BITS/4+2, fd)) { /* read one kernel cpu mask and the ending comma */
				unsigned long map = strtol(string, NULL, 16);
				memmove(&maps[1], &maps[0], (MAX_KERNEL_CPU_MASK-1)*sizeof(*maps));
				maps[0] = map;
				nr_maps++;
			}

			unsigned long mask;
			int mask_shift;
			for(k=0, mask=1, mask_shift=0; k<=nr_procs; ) {
				if (mask & maps[mask_shift]) {
					/* we found a cpu in the map */

					if (cacheids[level*MARCEL_NBMAXCPUS+k] != -1)
						/* already got this cache map, stop using it */
						break;

					/* this cpu didn't have any such cache yet, set this cache id for all cpus in the map */
					for(; k<=nr_procs; ) {
						if (mask & maps[mask_shift]) {
							mdebug("--- proc %d has L%d cache number %d\n", k, level+1, nr_caches[level]);
							cacheids[level*MARCEL_NBMAXCPUS+k] = nr_caches[level];
						}
						/* update cpu + mask + shift */
						if (!((++k)%KERNEL_CPU_MASK_BITS)) { mask_shift++; mask=1; } else { mask<<=1; }
					}

					/* increase the number of caches for this level and stop using this map */
					nr_caches[level]++;
					break;
				}

				/* update cpu + mask + shift */
				if (!((++k)%KERNEL_CPU_MASK_BITS)) { mask_shift++; mask=1; } else { mask<<=1; }
			}
			fclose(fd);
		}
	}
}

static void
ma_setup_cache_topo_level(int cachelevel, enum marcel_topo_level_e topotype, int nr_procs,
			  unsigned *numcaches, unsigned *cacheids)
{
	struct marcel_topo_level *level;
	int j,k;

	mdebug("%d L%d caches\n", cachelevel+1, numcaches[cachelevel]);
	level=__marcel_malloc((numcaches[cachelevel]+MARCEL_NBMAXVPSUP+1)*sizeof(*level));
	MA_BUG_ON(!level);

	for (j = 0; j < numcaches[cachelevel]; j++) {
		level[j].type = topotype;
		level[j].merged_type = 1<<topotype;
		switch (cachelevel) {
		case 2: ma_topo_set_os_numbers(&level[j], -1, -1, j, -1, -1, -1, -1); break;
		case 1: ma_topo_set_os_numbers(&level[j], -1, -1, -1, j, -1, -1, -1); break;
		case 0: ma_topo_set_os_numbers(&level[j], -1, -1, -1, -1, -1, j, -1); break;
		default: MA_BUG_ON(1);
		}
		marcel_vpset_zero(&level[j].vpset);
		marcel_vpset_zero(&level[j].cpuset);
		for (k=0; k <= nr_procs; k++)
			if (cacheids[cachelevel*MARCEL_NBMAXCPUS+k] == j)
				marcel_vpset_set(&level[j].cpuset,k);
		level[j].arity=0;
		level[j].children=NULL;
		level[j].father=NULL;
		mdebug("L%d cache %d has cpuset %"MA_VPSET_x" \t(%s)\n", cachelevel+1,
		       j, level[j].cpuset, tbx_i2smb(level[j].cpuset));
	}
	mdebug("\n");
	marcel_vpset_zero(&level[j].vpset);
	marcel_vpset_zero(&level[j].cpuset);
	marcel_topo_level_nbitems[discovering_level]=numcaches[cachelevel];
	mdebug("--- shared L%d level has number %d\n", cachelevel+1, discovering_level);
	marcel_topo_levels[discovering_level++]=level;
	mdebug("\n");
}

/* Look at Linux' /proc/cpuinfo */
static void __marcel_init look_cpuinfo(void) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	long processor=-1;
	unsigned proc_osphysid[MARCEL_NBMAXCPUS];
	unsigned proc_physid[MARCEL_NBMAXCPUS];
	unsigned osphysids[MARCEL_NBMAXCPUS];
	long physid;
	unsigned proc_oscoreid[MARCEL_NBMAXCPUS];
	unsigned proc_coreid[MARCEL_NBMAXCPUS];
	unsigned oscoreids[MARCEL_NBMAXCPUS];
	unsigned proc_cacheids[CACHE_LEVEL_MAX*MARCEL_NBMAXCPUS];
	/* Core numbers are not unique, we have to combine them with physical IDs */
	unsigned core_osphysids[MARCEL_NBMAXCPUS];
	long coreid;
	int i,j,k;

	unsigned numdies=0;
	unsigned numcores=0;
	unsigned numcaches[CACHE_LEVEL_MAX];

	if (!(fd=fopen("/proc/cpuinfo","r"))) {
		fprintf(stderr,"could not open /proc/cpuinfo\n");
		return;
	}

	memset(proc_osphysid,0,sizeof(proc_osphysid));
	memset(proc_physid,0,sizeof(proc_physid));
	memset(proc_oscoreid,0,sizeof(proc_oscoreid));
	memset(proc_coreid,0,sizeof(proc_coreid));

	/* Just record information and count number of dies and cores */

	/* TODO: use /sys/devices/system/cpu/cpu%d/topology/ instead:
	 * /core_siblings contains the map of cores in the die
	 * /thread_siblings contains the map of smtprocs in the core
	 *
	 * revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16)
	 */

	mdebug("\n\n * Topology extraction from /proc/cpuinfo *\n\n");
	while (fgets(string,sizeof(string),fd)!=NULL) {
#      define getprocnb_begin(field, var) \
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
#      define getprocnb_end() \
		}
		getprocnb_begin(PROCESSOR,processor);
		getprocnb_end() else
		getprocnb_begin(PHYSID,physid);
			proc_osphysid[processor]=physid;
			for (i=0; i<numdies; i++)
				if (physid == osphysids[i])
					break;
			proc_physid[processor]=i;
			mdebug("%ld on die %d (%lx)\n", processor, i, physid);
			if (i==numdies)
				osphysids[numdies++] = physid;
		getprocnb_end() else
		getprocnb_begin(COREID,coreid);
			proc_oscoreid[processor]=coreid;
			for (i=0; i<numcores; i++)
				if (coreid == oscoreids[i] && proc_osphysid[processor] == core_osphysids[i])
					break;
			proc_coreid[processor]=i;
			mdebug("%ld on core %d (%lx:%x)\n", processor, i, coreid, proc_osphysid[processor]);
			if (i==numcores) {
				core_osphysids[numcores] = proc_osphysid[processor];
				oscoreids[numcores] = coreid;
				numcores++;
			}
		getprocnb_end()
		if (string[strlen(string)-1]!='\n') {
			fscanf(fd,"%*[^\n]");
			getc(fd);
		}
	}
	fclose(fd);

	mdebug("\n\n * Topology summary *\n\n");
	mdebug("%ld processors\n", processor+1);

	if (numdies>1)
		mdebug("%d dies\n", numdies);
	if (numcores>1)
		mdebug("%d cores\n", numcores);

	struct marcel_topo_level *die_level;

	mdebug("\n\n * cpusets details *\n\n");
	if (numdies>1) {
		mdebug("%d dies\n", numdies);
		die_level=__marcel_malloc((numdies+MARCEL_NBMAXVPSUP+1)*sizeof(*die_level));
		MA_BUG_ON(!die_level);

		for (j = 0; j < numdies; j++) {
			die_level[j].type = MARCEL_LEVEL_DIE;
			die_level[j].merged_type = 1<<MARCEL_LEVEL_DIE;
			ma_topo_set_os_numbers(&die_level[j], -1, osphysids[j], -1, -1, -1, -1, -1);
			marcel_vpset_zero(&die_level[j].vpset);
			marcel_vpset_zero(&die_level[j].cpuset);
			for (k=0; k <= processor; k++)
				if (proc_osphysid[k] == osphysids[j])
					marcel_vpset_set(&die_level[j].cpuset,k);
			die_level[j].arity=0;
			die_level[j].children=NULL;
			die_level[j].father=NULL;
			mdebug("die %d has cpuset %"MA_VPSET_x" \t(%s)\n",j,die_level[j].cpuset,
					tbx_i2smb(die_level[j].cpuset));
		}
		mdebug("\n");

		marcel_vpset_zero(&die_level[j].vpset);
		marcel_vpset_zero(&die_level[j].cpuset);

		marcel_topo_level_nbitems[discovering_level]=numdies;
		mdebug("--- die level has number %d\n", discovering_level);
		marcel_topo_levels[discovering_level++]=die_level;
		mdebug("\n");
	}

	for(j=0; j<CACHE_LEVEL_MAX; j++) {
		numcaches[j] = 0;
		for(k=0; k<=processor; k++)
			proc_cacheids[j*MARCEL_NBMAXCPUS+k] = -1;
	}
	for(j=0; j<=processor; j++) {
		ma_parse_shared_cpu_maps(j, processor, proc_cacheids, numcaches);
	}

	if (numcaches[2] > 1) {
		/* setup L3 caches */
		ma_setup_cache_topo_level(2, MARCEL_LEVEL_L3, processor, numcaches, proc_cacheids);
	}

	if (numcaches[1] > 1) {
		/* setup L2 caches */
		ma_setup_cache_topo_level(1, MARCEL_LEVEL_L2, processor, numcaches, proc_cacheids);
	}

	struct marcel_topo_level *core_level;

	if (numcores>1) {
		mdebug("%d cores\n", numcores);
		core_level=__marcel_malloc((numcores+MARCEL_NBMAXVPSUP+1)*sizeof(*core_level));
		MA_BUG_ON(!core_level);

		for (j = 0; j < numcores; j++) {
			core_level[j].type = MARCEL_LEVEL_CORE;
			core_level[j].merged_type = 1<<MARCEL_LEVEL_CORE;
			ma_topo_set_os_numbers(&core_level[j], -1, -1, -1, -1, oscoreids[j], -1, -1);
			marcel_vpset_zero(&core_level[j].vpset);
			marcel_vpset_zero(&core_level[j].cpuset);
			for (k=0; k <= processor; k++)
				if (proc_oscoreid[k] == oscoreids[j] && proc_osphysid[k] == core_osphysids[j])
					marcel_vpset_set(&core_level[j].cpuset,k);
			core_level[j].arity=0;
			core_level[j].children=NULL;
			core_level[j].father=NULL;
#      ifdef MARCEL_SMT_IDLE
			ma_atomic_set(&core_level[j].nbidle, 0);
#      endif
			mdebug("core %d has cpuset %"MA_VPSET_x" \t(%s)\n",j,core_level[j].cpuset,
					tbx_i2smb(core_level[j].cpuset));
		}

		mdebug("\n");

		marcel_vpset_zero(&core_level[j].vpset);
		marcel_vpset_zero(&core_level[j].cpuset);

		marcel_topo_level_nbitems[discovering_level]=numcores;
		mdebug("--- core level has number %d\n", discovering_level);
#      ifdef MARCEL_SMT_IDLE
		marcel_topo_core_level =
#      endif
		marcel_topo_levels[discovering_level++]=core_level;
		mdebug("\n");
	}

	if (numcaches[0] > 1) {
		/* setup L1 caches */
		ma_setup_cache_topo_level(0, MARCEL_LEVEL_L1, processor, numcaches, proc_cacheids);
	}
}
#    endif /* LINUX_SYS */

#    ifdef LINUX_SYS
#      include <numa.h>
/* Ask libnuma for NUMA nodes */
static void __marcel_init look_libnuma(void) {
	unsigned long *buffer,*buffer2;
	unsigned buffersize=tbx_max(MARCEL_NBMAXCPUS/8,8); /* make sure libnuma v2 won't complain about our buffer size */
	unsigned i,j;
	unsigned nbnodes;
	struct marcel_topo_level *node_level;
	marcel_vpset_t cpuset;

	numa_exit_on_error=1;

	if ((ma_numa_not_available = (numa_available())==-1))
		return;

	nbnodes=numa_max_node()+1;
	if (nbnodes==1) {
		nbnodes=0;
		ma_numa_not_available=1;
		return;
	}

	MA_BUG_ON(nbnodes==0);

	node_level=__marcel_malloc((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*node_level));
	MA_BUG_ON(!node_level);

	if (!(buffer=TBX_MALLOC(buffersize))) {
		fprintf(stderr,"no room for storing cpu set\n");
		return;
	}

	while (numa_node_to_cpus(0, buffer, buffersize)==-1 && errno==ERANGE) {
		buffersize*=2;
		if (!(buffer2 = TBX_REALLOC(buffer, buffersize))) {
			fprintf(stderr,"no room for storing cpu set\n");
			TBX_FREE(buffer);
			return;
		}
		buffer=buffer2;
	}

	/* TODO: use /sys/devices/system/node/node%d/cpumap ? it exists since 2.6.0 */

	for (i=0;i<nbnodes;i++) {
		if (numa_node_to_cpus(i, buffer, buffersize)) {
			fprintf(stderr,"numa_node_to_cpus(%d) failed: %s\n",i,strerror(errno));
			continue;
		}
		node_level[i].type = MARCEL_LEVEL_NODE;
		node_level[i].merged_type = 1<<MARCEL_LEVEL_NODE;
		ma_topo_set_os_numbers(&node_level[i], i, -1, -1, -1, -1, -1, -1);
		marcel_vpset_zero(&node_level[i].vpset);
		node_level[i].cpuset=cpuset=buffer[0];
		mdebug("node %d has cpuset %"MA_VPSET_x"\n",i,cpuset);
		for (j=0;j<marcel_nbvps();j++)
			if (marcel_vpset_isset(&cpuset,j))
				ma_vp_node[j]=i;
		node_level[i].arity=0;
		node_level[i].children=NULL;
		node_level[i].father=NULL;
	}

	marcel_vpset_zero(&node_level[i].vpset);
	marcel_vpset_zero(&node_level[i].cpuset);

	marcel_topo_level_nbitems[discovering_level] = marcel_nbnodes = nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;

	TBX_FREE(buffer);
}
#    endif /* LINUX_SYS */


#    ifdef OSF_SYS
#       include <numa.h>
/* Ask libnuma for topology */
static void __marcel_init look_libnuma(void) {
	cpu_cursor_t cursor;
	unsigned i = 0;
	unsigned nbnodes;
	radid_t radid;
	cpuid_t cpuid;
	cpuset_t cpuset;
	struct marcel_topo_level *node_level;

	nbnodes=rad_get_num();
	if (nbnodes==1) {
		nbnodes=0;
		return;
	}

	MA_BUG_ON(nbnodes==0);

	node_level=__marcel_malloc((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*node_level));
	MA_BUG_ON(!node_level);

	cpusetcreate(&cpuset);
	for (radid = 0; radid < nbnodes; radid++) {
		cpuemptyset(cpuset);
		if (rad_get_cpus(radid, cpuset)==-1) {
			fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
			continue;
		}

		node_level[i].type = MARCEL_LEVEL_NODE;
		node_level[i].merged_type = 1<<MARCEL_LEVEL_NODE;
		ma_topo_set_os_numbers(&node_level[i], radid, -1, -1, -1, -1, -1, -1);
		marcel_vpset_zero(&node_level[i].vpset);
		marcel_vpset_zero(&node_level[i].cpuset);
		cursor = SET_CURSOR_INIT;
		while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
			marcel_vpset_set(&node_level[i].cpuset,cpuid);
		mdebug("node %d has cpuset %"MA_VPSET_x"\n",i,node_level[i].cpuset);
		node_level[i].arity=0;
		node_level[i].children=NULL;
		node_level[i].father=NULL;
		i++;
	}

	marcel_vpset_zero(&node_level[i].vpset);
	marcel_vpset_zero(&node_level[i].cpuset);

	marcel_topo_level_nbitems[discovering_level] = marcel_nbnodes = nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;
}
#    endif /* OSF_SYS */

#    ifdef AIX_SYS
#      include <sys/rset.h>
/* Ask rsets for topology */
static void __marcel_init look_rset(int sdl, enum marcel_topo_level_e level) {
	rsethandle_t rset, rad;
	int r,i,maxcpus,j;
	unsigned nbnodes;
	struct marcel_topo_level *rad_level;

	rset = rs_alloc(RS_PARTITION);
	rad = rs_alloc(RS_EMPTY);
	nbnodes = rs_numrads(rset, sdl, 0);
	if (nbnodes == -1) {
		perror("rs_numrads");
		return;
	}
	if (nbnodes == 1)
		return;

	MA_BUG_ON(nbnodes == 0);

	rad_level=__marcel_malloc((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*rad_level));
	MA_BUG_ON(!rad_level);

	for (r = 0, i = 0; i < nbnodes; i++) {
		if (rs_getrad(rset, rad, sdl, i, 0)) {
			fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
			continue;
		}
		if (!rs_getinfo(rad, R_NUMPROCS, 0))
			continue;

		rad_level[r].type = level;
		ma_topo_set_os_numbers(&rad_level[r], -1, -1, -1, -1, -1, -1, -1);
		switch(level) {
			case MARCEL_LEVEL_NODE:
				rad_level[r].os_node = r;
				break;
			case MARCEL_LEVEL_L2:
				rad_level[r].os_l2 = r;
				break;
			case MARCEL_LEVEL_CORE:
				rad_level[r].os_core = r;
				break;
			case MARCEL_LEVEL_PROC:
				rad_level[r].os_cpu = r;
				break;
			default:
				break;
		}
		marcel_vpset_zero(&rad_level[r].vpset);
		marcel_vpset_zero(&rad_level[r].cpuset);
		maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
		for (j = 0; j < maxcpus; j++) {
			if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
				marcel_vpset_set(&rad_level[r].cpuset,j);
		}
		mdebug("node %d has cpuset %"MA_VPSET_x"\n",r,rad_level[r].cpuset);
		rad_level[r].arity=0;
		rad_level[r].children=NULL;
		rad_level[r].father=NULL;
#      ifdef MARCEL_SMT_IDLE
		if (level == MARCEL_LEVEL_CORE)
			ma_atomic_set(&rad_level[r].nbidle, 0);
#      endif
		r++;
	}

	marcel_vpset_zero(&rad_level[r].vpset);
	marcel_vpset_zero(&rad_level[r].cpuset);

	marcel_topo_level_nbitems[discovering_level] = nbnodes;
	marcel_topo_levels[discovering_level++] = rad_level;
	if (level == MARCEL_LEVEL_NODE) {
		marcel_topo_node_level = rad_level;
		marcel_nbnodes = nbnodes;
	}
#      ifdef MARCEL_SMT_IDLE
	if (level == MARCEL_LEVEL_CORE)
		marcel_topo_core_level = rad_level;
#      endif
	rs_free(rset);
	rs_free(rad);
}
#    endif /* AIX_SYS */

#    ifdef WIN_SYS
#      warning TODO: use GetLogicalProcessorInformation, GetNumaHighestNodeNumber, and GetNumaNodeProcessorMask
#    endif


/* Use the value returned by ma_nbprocessors() */
static void look_cpu(void) {
	struct marcel_topo_level *cpu_level;
	unsigned cpu;

	cpu_level=__marcel_malloc((marcel_nbprocessors+MARCEL_NBMAXVPSUP+1)*sizeof(*cpu_level));
	MA_BUG_ON(!cpu_level);

	mdebug("\n\n * CPU cpusets *\n\n");
	for (cpu=0; cpu<marcel_nbprocessors; cpu++) {
		cpu_level[cpu].type=MARCEL_LEVEL_PROC;
		cpu_level[cpu].merged_type = 1<<MARCEL_LEVEL_PROC;
		ma_topo_set_os_numbers(&cpu_level[cpu], -1, -1, -1, -1, -1, -1, cpu);
		marcel_vpset_zero(&cpu_level[cpu].vpset);
		marcel_vpset_vp(&cpu_level[cpu].cpuset, cpu);
		mdebug("cpu %d has cpuset %"MA_VPSET_x" \t(%s)\n",cpu,cpu_level[cpu].cpuset, tbx_i2smb(cpu_level[cpu].cpuset));
		cpu_level[cpu].arity=0;
		cpu_level[cpu].children=NULL;
		cpu_level[cpu].father=NULL;
	}
	marcel_vpset_zero(&cpu_level[cpu].vpset);
	marcel_vpset_zero(&cpu_level[cpu].cpuset);

	marcel_topo_level_nbitems[discovering_level]=marcel_nbprocessors;
	marcel_topo_levels[discovering_level++]=
		marcel_topo_cpu_level = cpu_level;
}
#  endif /* MA__NUMA */

#  ifdef MA__NUMA
/* split arity into nbsublevels of size sublevelarity */
static void split(unsigned arity, unsigned * __restrict sublevelarity, unsigned * __restrict nbsublevels) {
	*sublevelarity = sqrt(arity);
	*nbsublevels = (arity + *sublevelarity-1) / *sublevelarity;
	if (*nbsublevels > marcel_topo_max_arity) {
		*nbsublevels = marcel_topo_max_arity;
		*sublevelarity = (arity + *nbsublevels-1) / *nbsublevels;
	}
}
#  endif

/* Connect levels */
static void topo_connect(void) {
	int l, i, j, m;
	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].cpuset; i++) {
			if (marcel_topo_levels[l][i].arity) {
				mdebug("level %u,%u: cpuset %"MA_VPSET_x" arity %u\n",l,i,marcel_topo_levels[l][i].cpuset,marcel_topo_levels[l][i].arity);
				marcel_topo_levels[l][i].children=TBX_MALLOC(marcel_topo_levels[l][i].arity*sizeof(void *));
				MA_BUG_ON(!marcel_topo_levels[l][i].children);

				m=0;
				for (j=0; marcel_topo_levels[l+1][j].cpuset; j++)
					if (!(marcel_topo_levels[l+1][j].cpuset &
						~(marcel_topo_levels[l][i].cpuset))) {
						MA_BUG_ON(m >= marcel_topo_levels[l][i].arity);
						marcel_topo_levels[l][i].children[m]=
							&marcel_topo_levels[l+1][j];
						marcel_topo_levels[l+1][j].father = &marcel_topo_levels[l][i];
						marcel_topo_levels[l+1][j].index = m++;
					}
			}
		}
	}
}

#  ifdef MA__NUMA
static int compar(const void *_l1, const void *_l2) {
  const struct marcel_topo_level *l1 = _l1, *l2 = _l2;
  return ma_ffs(l1->cpuset) - ma_ffs(l2->cpuset);
}
#  endif

/* Main discovery loop */
static void topo_discover(void) {
	unsigned l,i,j;
#  ifdef MA__NUMA
	unsigned m,n;
#  endif
	struct marcel_topo_level *level;

	if (marcel_nbvps() + MARCEL_NBMAXVPSUP > MA_NR_LWPS) {
		fprintf(stderr,"%d > %d, please increase MARCEL_NBMAXCPUS in marcel/include/marcel_config.h\n", marcel_nbvps() + MARCEL_NBMAXVPSUP, MA_NR_LWPS);
		exit(1);
	}

	/* Raw detection, from coarser levels to finer levels */
#  ifdef MA__NUMA
	unsigned k;
	unsigned nbsublevels;
	unsigned sublevelarity;
	int dosplit;

	for (i = 0; i < marcel_nbvps() + MARCEL_NBMAXVPSUP; i++)
		ma_vp_node[i]=-1;

#    ifdef LINUX_SYS
	look_libnuma();
	look_cpuinfo();
#    endif
#    ifdef OSF_SYS
	look_libnuma();
#    endif
#    ifdef  AIX_SYS
	for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++) {
		if (i == rs_getinfo(NULL, R_MCMSDL, 0)) {
			mdebug("looking AIX node sdl %d\n",i);
			look_rset(i, MARCEL_LEVEL_NODE);
		}
#      ifdef R_L2CSDL
		if (i == rs_getinfo(NULL, R_L2CSDL, 0)) {
			mdebug("looking AIX L2 sdl %d\n",i);
			look_rset(i, MARCEL_LEVEL_L2);
		}
#      endif
#      ifdef R_PCORESDL
		if (i == rs_getinfo(NULL, R_PCORESDL, 0)) {
			mdebug("looking AIX core sdl %d\n",i);
			look_rset(i, MARCEL_LEVEL_CORE);
		}
#      endif
		if (i == rs_getinfo(NULL, R_SMPSDL, 0))
			mdebug("not looking AIX \"SMP\" sdl %d\n",i);
		if (i == rs_getinfo(NULL, R_MAXSDL, 0)) {
			mdebug("looking AIX max sdl %d\n",i);
			look_rset(i, MARCEL_LEVEL_PROC);
		}
	}
#    endif /* AIX_SYS */
	look_cpu();
#  endif /* MA__NUMA */

	marcel_topo_nblevels=discovering_level;
	mdebug("\n\n--> discovered %d levels\n\n", marcel_topo_nblevels);

#  ifdef MA__NUMA

	/* sort levels according to cpu sets */
	for (l=0; l+1<marcel_topo_nblevels; l++) {
		/* first sort sublevels according to cpu sets */
		qsort(&marcel_topo_levels[l+1][0], marcel_topo_level_nbitems[l+1], sizeof(*marcel_topo_levels[l+1]), compar);
		k = 0;
		/* then gather sublevels according to levels */
		for (i=0; i<marcel_topo_level_nbitems[l]; i++) {
			marcel_vpset_t level_set = marcel_topo_levels[l][i].cpuset;
			for (j=k; j<marcel_topo_level_nbitems[l+1]; j++) {
				marcel_vpset_t set = level_set;
				set = set & marcel_topo_levels[l+1][j].cpuset;
				if (set) {
					/* Sublevel j is part of level i, put it at k.  */
					struct marcel_topo_level level = marcel_topo_levels[l+1][j];
					memmove(&marcel_topo_levels[l+1][k+1], &marcel_topo_levels[l+1][k], (j-k)*sizeof(*marcel_topo_levels[l+1]));
					marcel_topo_levels[l+1][k++] = level;
				}
			}
		}
	}
#  endif /* MA__NUMA */

	/* TODO: Brice will probably want the OS->VP function */

	/* create the VP level: now we decide which CPUs we will really use according to nvp and stride */
	struct marcel_topo_level *vp_level = __marcel_malloc((marcel_nbvps()+MARCEL_NBMAXVPSUP+1)*sizeof(*vp_level));
	MA_BUG_ON(!vp_level);

	marcel_vpset_t cpuset = MARCEL_VPSET_ZERO;
	/* do not initialize supplementary VPs yet, since they may end up on the machine level on a single-CPU machine */
	unsigned cpu = marcel_first_cpu;
	unsigned vps = 0;

	mdebug("\n\n * VP mapping details *\n\n");
	for (i=0; i<marcel_nbvps(); i++)  {
#  ifdef MA__NUMA
		MA_BUG_ON(cpu>=marcel_nbprocessors);
		marcel_vpset_t oscpuset = marcel_topo_levels[marcel_topo_nblevels-1][cpu].cpuset;
		MA_BUG_ON(marcel_vpset_weight(&oscpuset) != 1);
		unsigned oscpu = ma_ffs(oscpuset)-1;
#  else
		unsigned oscpu = cpu;
#  endif
		vp_level[i].type=MARCEL_LEVEL_VP;
		vp_level[i].merged_type=1<<MARCEL_LEVEL_VP;
		ma_topo_set_os_numbers(&vp_level[i], -1, -1, -1, -1, -1, -1, oscpu);
		marcel_vpset_vp(&vp_level[i].cpuset, oscpu);
		marcel_vpset_set(&cpuset, oscpu);
		marcel_vpset_zero(&vp_level[i].vpset);
		vp_level[i].arity=0;
		vp_level[i].children=NULL;
		vp_level[i].father=NULL;
		mdebug("VP %d on %dth proc with cpuset %"MA_VPSET_x" \t(%s)\n",i,cpu,vp_level[i].cpuset, tbx_i2smb(vp_level[i].cpuset));

		/* Follow the machine as nicely as possible, for instance, with two bicore chips and 6 vps:
		 * chips: [     ] [     ] *
		 * cpus:  [ ] [ ] [ ] [ ] *
		 * VPs:    0   2   4   5  *
		 *         1   3          */
		int stopfill = i >= (((marcel_nbvps()-1)%marcel_nbprocessors+1) * marcel_vps_per_cpu);
		if (++vps == marcel_vps_per_cpu - stopfill) {
			cpu += marcel_cpu_stride;
			vps = 0;
		}
	}
	mdebug("\n\n");
	marcel_vpset_zero(&vp_level[i].cpuset);
	marcel_vpset_zero(&vp_level[i].vpset);

	/* Now see where we will put it. Usually marcel_cpu_stride is 1 and hence we get at bottom */
	for (l=marcel_topo_nblevels-1; l>0; l--) {
		if (marcel_vpset_weight(&marcel_topo_levels[l][0].cpuset) >= marcel_cpu_stride) {
			break;
		}
	}
	l++;
	/* Discard levels below */
	while (marcel_topo_nblevels-1 >= l) {
#  ifdef MA__NUMA
		if (marcel_topo_levels[marcel_topo_nblevels-1] == marcel_topo_cpu_level)
			marcel_topo_cpu_level = NULL;
#    ifdef MARCEL_SMT_IDLE
		if (marcel_topo_levels[marcel_topo_nblevels-1] == marcel_topo_core_level)
			marcel_topo_core_level = NULL;
#    endif
		if (marcel_topo_levels[marcel_topo_nblevels-1] == marcel_topo_node_level)
			marcel_topo_node_level = NULL;
#  endif
		__marcel_free(marcel_topo_levels[marcel_topo_nblevels-1]);
		marcel_topo_nblevels--;
		marcel_topo_levels[marcel_topo_nblevels] = NULL;
	}

	/* And add this one */
	marcel_topo_level_nbitems[marcel_topo_nblevels] = marcel_nbvps();
	marcel_topo_levels[marcel_topo_nblevels++] = vp_level;

	/* Now filter out CPUs from levels. */
	for (l=0; l<marcel_topo_nblevels; l++) {
		for (i=0; i<marcel_topo_level_nbitems[l]; i++) {
			marcel_topo_levels[l][i].cpuset &= cpuset;
			mdebug("level %u,%u: cpuset becomes %"MA_VPSET_x"\n", l, i, marcel_topo_levels[l][i].cpuset);
			if (!marcel_topo_levels[l][i].cpuset) {
				mdebug("became empty, dropping it\n");
				marcel_topo_level_nbitems[l]--;
				memcpy(&marcel_topo_levels[l][i], &marcel_topo_levels[l][i+1], sizeof(marcel_topo_levels[l][i]) * (marcel_topo_level_nbitems[l]-i+1));
				i--;
			}
		}
	}

	/* Now we can put numbers on levels. */
	for (l=0; l<marcel_topo_nblevels; l++)
		for (i=0; i<marcel_topo_level_nbitems[l]; i++) {
			marcel_topo_levels[l][i].number = i;
			mdebug("level %u,%u: cpuset %"MA_VPSET_x"\n",l,i,marcel_topo_levels[l][i].cpuset);
		}

	/* And show debug again */
	for (l=0; l<marcel_topo_nblevels; l++)
		for (i=0; i<marcel_topo_level_nbitems[l]; i++)
			mdebug("level %u,%u: cpuset %"MA_VPSET_x"\n",l,i,marcel_topo_levels[l][i].cpuset);

#  ifdef MA__NUMA
	for (l=0; l <= MARCEL_LEVEL_LAST; l++)
	  ma_topo_type_level[l] = -1;
	/* merge identical levels */
	for (l=0; l+1<marcel_topo_nblevels; l++) {
	  ma_topo_type_level[marcel_topo_levels[l][0].type] = l;
		for (i=0; marcel_topo_levels[l][i].cpuset; i++);
		for (j=0; j<i && marcel_topo_levels[l+1][j].cpuset; j++)
			if (marcel_topo_levels[l+1][j].cpuset != marcel_topo_levels[l][j].cpuset)
				break;
		if (j==i && !marcel_topo_levels[l+1][j].cpuset) {
			mdebug("merging levels %u and %u since same %d item%s\n", l, l+1, i, i>=2?"s":"");
			if (marcel_topo_levels[l+1] == marcel_topo_cpu_level)
				marcel_topo_cpu_level = marcel_topo_levels[l];
#    ifdef MARCEL_SMT_IDLE
			else if (marcel_topo_levels[l+1] == marcel_topo_core_level)
				marcel_topo_core_level = marcel_topo_levels[l];
#    endif
			else if (marcel_topo_levels[l+1] == marcel_topo_node_level)
				marcel_topo_node_level = marcel_topo_levels[l];
			for (i=0; i<marcel_topo_level_nbitems[l]; i++) {
				marcel_topo_levels[l][i].merged_type |= marcel_topo_levels[l+1][i].merged_type;
#    define merge_os_components(component) \
				if (marcel_topo_levels[l][i].os_##component == -1) \
					marcel_topo_levels[l][i].os_##component = marcel_topo_levels[l+1][i].os_##component; \
				else \
					if (marcel_topo_levels[l+1][i].os_##component != -1) \
						MA_BUG_ON(marcel_topo_levels[l][i].os_##component != marcel_topo_levels[l+1][i].os_##component);
				merge_os_components(node);
				merge_os_components(die);
				merge_os_components(l3);
				merge_os_components(l2);
				merge_os_components(core);
				merge_os_components(l1);
				merge_os_components(cpu);
			}
			__marcel_free(marcel_topo_levels[l+1]);
			memmove(&marcel_topo_level_nbitems[l+1],&marcel_topo_level_nbitems[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_level_nbitems));
			memmove(&marcel_topo_levels[l+1],&marcel_topo_levels[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_levels));
			marcel_topo_nblevels--;
			marcel_topo_levels[marcel_topo_nblevels] = NULL;
			l--;
		}
	}
	int type, previous_level = -1;
	for (type = 0; type <= MARCEL_LEVEL_LAST; type++) {
	  if (ma_topo_type_level[type] == -1)
	    ma_topo_type_level[type] = previous_level;
	  else
	    previous_level = ma_topo_type_level[type];
	}
#  endif /* MA__NUMA */

	/* Compute arity */
	for (l=0; l+1<marcel_topo_nblevels; l++) {
		for (i=0; marcel_topo_levels[l][i].cpuset; i++) {
			marcel_topo_levels[l][i].arity=0;
			for (j=0; marcel_topo_levels[l+1][j].cpuset; j++)
				if (!(marcel_topo_levels[l+1][j].cpuset &
					~(marcel_topo_levels[l][i].cpuset)))
					marcel_topo_levels[l][i].arity++;
			mdebug("level %u,%u: cpuset %"MA_VPSET_x" \t(%s) arity %u\n",l,i,marcel_topo_levels[l][i].cpuset,tbx_i2smb(marcel_topo_levels[l][i].cpuset),marcel_topo_levels[l][i].arity);
		}
	}
	for (i=0; marcel_topo_levels[marcel_topo_nblevels-1][i].cpuset; i++)
		mdebug("level %u,%u: cpuset %"MA_VPSET_x" \t(%s) leaf\n",marcel_topo_nblevels-1,i,marcel_topo_levels[marcel_topo_nblevels-1][i].cpuset,tbx_i2smb(marcel_topo_levels[marcel_topo_nblevels-1][i].cpuset));
	mdebug("arity done.\n");

	/* and finally connect levels */
	topo_connect();
	mdebug("connecting done.\n");

#  ifdef MA__NUMA
	/* Split hi-arity levels */
	if (marcel_topo_max_arity) {
		/* For each level */
		for (l=0; l+1<marcel_topo_nblevels; l++) {
			unsigned level_width = 0;
			dosplit = 0;
			/* Look at each item, check for max_arity */
			for (i=0; marcel_topo_levels[l][i].cpuset; i++) {
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
				marcel_topo_levels[l+1] = __marcel_malloc((level_width+MARCEL_NBMAXVPSUP+1)*sizeof(**marcel_topo_levels));
				/* fill it with split items */
				for (i=0,j=0; marcel_topo_levels[l][i].cpuset; i++) {
					/* split one item */
					split(marcel_topo_levels[l][i].arity,&sublevelarity,&nbsublevels);
					mdebug("splitting level %u,%u into %u sublevels of size at most %u\n", l, i, nbsublevels, sublevelarity);
					/* initialize subitems */
					for (k=0; k<nbsublevels; k++) {
						level = &marcel_topo_levels[l+1][j+k];
						level->type = MARCEL_LEVEL_FAKE;
						level->merged_type = 1<<MARCEL_LEVEL_FAKE;
						level->number = j+k;
						level->index = k;
						ma_topo_set_os_numbers(level, -1, -1, -1, -1, -1, -1, -1);
						marcel_vpset_zero(&level->cpuset);
						marcel_vpset_zero(&level->vpset);
						level->arity = 0;
						level->children = TBX_MALLOC(sublevelarity*sizeof(void*));
						level->father = &marcel_topo_levels[l][i];
					}

					/* distribute cpus to subitems */
					/* give cpus of sublevelarity items to each fake item */

					/* first fake item */
					k = 0;
					/* will get first item's cpus */
					m = 0;
					for (n=0; n<marcel_topo_levels[l][i].arity; n++) {
						level = &marcel_topo_levels[l+1][j+k];
						level->cpuset |=
							marcel_topo_levels[l][i].children[n]->cpuset;
						level->arity++;
						level->children[m] = marcel_topo_levels[l][i].children[n];
						marcel_topo_levels[l][i].children[n]->index = m;
						marcel_topo_levels[l][i].children[n]->father = level;
						if (++m == sublevelarity) {
							k++;
							m = 0;
						}
					}
					if (m)
						/* Incomplete last level */
						k++;
					MA_BUG_ON(k!=nbsublevels);

					/* reconnect */
					marcel_topo_levels[l][i].arity = nbsublevels;
					TBX_FREE(marcel_topo_levels[l][i].children);
					marcel_topo_levels[l][i].children = TBX_MALLOC(nbsublevels*sizeof(void*));
					for (k=0; k<nbsublevels; k++)
						marcel_topo_levels[l][i].children[k] = &marcel_topo_levels[l+1][j+k];

					mdebug("now level %u,%u: cpuset %"MA_VPSET_x" has arity %u\n",l,i,marcel_topo_levels[l][i].cpuset,marcel_topo_levels[l][i].arity);
					j += nbsublevels;
				}
				MA_BUG_ON(j!=level_width);
				marcel_vpset_zero(&marcel_topo_levels[l+1][j].cpuset);
				marcel_vpset_zero(&marcel_topo_levels[l+1][j].vpset);
				if (++marcel_topo_nblevels ==
					sizeof(marcel_topo_levels)/sizeof(*marcel_topo_levels))
					MA_BUG();
			}
		}
	}
#  endif /* MA__NUMA */

	MA_BUG_ON(marcel_topo_level_nbitems[marcel_topo_nblevels-1] != marcel_nbvps());
	marcel_topo_vp_level = marcel_topo_levels[marcel_topo_nblevels-1];

	/* Set VP sets */
	/* This is the only one which isn't empty by default */
	marcel_vpset_zero(&marcel_machine_level[0].vpset);
	for (i=0; i<marcel_nbvps(); i++) {
		struct marcel_topo_level *level = &marcel_topo_vp_level[i];
		while (level) {
			marcel_vpset_set(&level->vpset, i);
			level = level->father;
		}
	}

	/* Now add supplementary VPs on the last level. */
	for (i=marcel_nbvps(); i<marcel_nbvps() + MARCEL_NBMAXVPSUP; i++) {
		marcel_topo_vp_level[i].type=MARCEL_LEVEL_VP;
		marcel_topo_vp_level[i].merged_type=1<<MARCEL_LEVEL_VP;
		marcel_topo_vp_level[i].number=i;
		ma_topo_set_os_numbers(&marcel_topo_vp_level[i], -1, -1, -1, -1, -1, -1, -1);
		marcel_vpset_vp(&marcel_topo_vp_level[i].vpset,i);
		marcel_vpset_zero(&marcel_topo_vp_level[i].cpuset);
		marcel_topo_vp_level[i].arity=0;
		marcel_topo_vp_level[i].children=NULL;
		marcel_topo_vp_level[i].father=NULL;
	}
	marcel_vpset_zero(&marcel_topo_vp_level[i].cpuset);
	marcel_vpset_zero(&marcel_topo_vp_level[i].vpset);

	for (level = &marcel_topo_vp_level[0]; level < &marcel_topo_vp_level[marcel_nbvps() + MARCEL_NBMAXVPSUP]; level++)
		level->vpdata = (struct marcel_topo_vpdata) MARCEL_TOPO_VPDATA_INITIALIZER(&level->vpdata);
#  ifdef MA__NUMA
	if (marcel_topo_node_level)
#  endif
		for (level = &marcel_topo_node_level[0]; level->vpset; level++)
			level->nodedata = (struct marcel_topo_nodedata) MARCEL_TOPO_NODEDATA_INITIALIZER(&level->nodedata);

	for (l=0; l<marcel_topo_nblevels; l++)
		for (i=0; marcel_topo_levels[l][i].vpset; i++)
			marcel_topo_levels[l][i].level = l;

	/* Now we can set VP levels for main LWP */
	ma_init_lwp_vpnum(0, &__main_lwp);
}

void ma_topo_exit(void) {
	unsigned l,i;
	/* Last level is not freed because we need it in various places */
	for (l=0; l<marcel_topo_nblevels-1; l++) {
		for (i=0; marcel_topo_levels[l][i].cpuset; i++) {
			TBX_FREE(marcel_topo_levels[l][i].children);
			marcel_topo_levels[l][i].children = NULL;
		}
		if (l) {
			__marcel_free(marcel_topo_levels[l]);
			marcel_topo_levels[l] = NULL;
		}
	}
}

__ma_initfunc(topo_discover, MA_INIT_TOPOLOGY, "Finding Topology");

static void topology_lwp_init(ma_lwp_t lwp) {
	/* FIXME: node_level/core_level/cpu_level are not associated with LWPs but VPs ! */
#  ifdef MA__NUMA
	int i;
	if (marcel_topo_node_level) {
		for (i=0; marcel_topo_node_level[i].cpuset; i++) {
			if (marcel_vpset_isset(&marcel_topo_node_level[i].cpuset,ma_vpnum(lwp))) {
				ma_per_lwp(node_level,lwp) = &marcel_topo_node_level[i];
				break;
			}
		}
	}
#    ifdef MARCEL_SMT_IDLE
	if (marcel_topo_core_level) {
		for (i=0; marcel_topo_core_level[i].cpuset; i++) {
			if (marcel_vpset_isset(&marcel_topo_core_level[i].cpuset,ma_vpnum(lwp))) {
				ma_per_lwp(core_level,lwp) = &marcel_topo_core_level[i];
				break;
			}
		}
	}
#    endif /* MARCEL_SMT_IDLE */
	if (marcel_topo_cpu_level) {
		for (i=0; marcel_topo_cpu_level[i].cpuset; i++) {
			if (marcel_vpset_isset(&marcel_topo_cpu_level[i].cpuset,ma_vpnum(lwp))) {
				ma_per_lwp(cpu_level,lwp) = &marcel_topo_cpu_level[i];
				break;
			}
		}
	}
#  endif /* MA__NUMA */
}

static void topology_lwp_start(ma_lwp_t lwp) {
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(topology, 400, "Topology",
				  topology_lwp_init, "Initialisation de la topologie",
				  topology_lwp_start, "Activation de la topologie");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(topology, MA_INIT_TOPOLOGY);

#endif /* MA__LWPS */
