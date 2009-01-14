
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

#define _ATFILE_SOURCE
#include "marcel.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>

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


/* Getting the number of processors.  */

#ifdef MA__LWPS

/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `ma_topology_set_fsys_root ()' to
   have the desired effect.  */
static unsigned ma_fallback_nbprocessors(void) {
#if defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
	return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
	return sysconf(_SC_NPROC_CONF);
#elif defined(DARWIN_SYS)
	struct host_basic_info info;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	host_info(mach_host_self(), HOST_BASIC_INFO, (integer_t*) &info, &count);
	return info.avail_cpus;
#else
#warning No known way to discover number of available processors on this system
#warning ma_fallback_nbprocessors will default to 1
#warning use the --marcel-nvp option to set it by hand when running your program
	return 1;
#endif
}

#endif


/* Support for synthetic topologies.  */

#ifdef MA__NUMA
unsigned   ma_synthetic_topology_description[MA_SYNTHETIC_TOPOLOGY_MAX_DEPTH];
tbx_bool_t ma_use_synthetic_topology = tbx_false;
#endif



#undef marcel_current_vp
unsigned marcel_current_vp(void)
{
	return __marcel_current_vp();
}

unsigned marcel_current_node(void)
{
#ifdef MA__NUMA
  unsigned vp = marcel_current_vp();
  return ma_vp_node[vp];
#else
  return 0;
#endif
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

static void ma_print_level_description(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode)
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

#define marcel_memory_size_printf_value(_size) \
  (_size)%1024 ? (_size) : (_size)%(1024*1024) ? (_size)>>10 : (_size)>>20
#define marcel_memory_size_printf_unit(_size) \
  (_size)%1024 ? "kB" : (_size)%(1024*1024) ? "MB" : "GB"

void marcel_print_level(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode,
			const char *separator, const char *indexprefix, const char* labelseparator, const char* levelterm) {
  ma_print_level_description(l, output, txt_mode, verbose_mode);
  marcel_fprintf(output, "%s", labelseparator);
#ifdef MA__NUMA
  if (l->os_node != -1) marcel_fprintf(output, "%sNode%s%u(%ld%s)", separator, indexprefix, l->os_node,
				       marcel_memory_size_printf_value(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE]),
				       marcel_memory_size_printf_unit(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE]));
  if (l->os_die != -1)  marcel_fprintf(output, "%sDie%s%u" , separator, indexprefix, l->os_die);
  if (l->os_l3 != -1)   marcel_fprintf(output, "%sL3%s%u(%ld%s)", separator, indexprefix, l->os_l3,
				       marcel_memory_size_printf_value(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L3]),
				       marcel_memory_size_printf_unit(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L3]));
  if (l->os_l2 != -1)   marcel_fprintf(output, "%sL2%s%u(%ld%s)", separator, indexprefix, l->os_l2,
				       marcel_memory_size_printf_value(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L2]),
				       marcel_memory_size_printf_unit(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L2]));
  if (l->os_core != -1) marcel_fprintf(output, "%sCore%s%u", separator, indexprefix, l->os_core);
  if (l->os_l1 != -1)   marcel_fprintf(output, "%sL1%s%u(%ld%s)", separator, indexprefix, l->os_l1,
				       marcel_memory_size_printf_value(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L1]),
				       marcel_memory_size_printf_unit(l->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L1]));
  if (l->os_cpu != -1)  marcel_fprintf(output, "%sCPU%s%u" , separator, indexprefix, l->os_cpu);
#endif
  if (l->level == marcel_topo_nblevels-1) {
    marcel_fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  marcel_fprintf(output,"%s\n", levelterm);
}

unsigned marcel_nbnodes = 1;


#ifdef __GLIBC__
# define HAVE_OPENAT
#endif

#ifdef HAVE_OPENAT

/* The file descriptor for the file system root, used when browsing, e.g.,
	 Linux' sysfs and procfs.  */
static int fsys_root_fd = -1;

int ma_topology_set_fsys_root(const char *path) {
	int root;

	root = open(path, O_RDONLY | O_DIRECTORY);
	if (root < 0)
		return -1;

	fsys_root_fd = root;
	return 0;
}

static FILE *ma_fopenat(const char *path, const char *mode) {
	int fd;
	const char *relative_path;

	MA_BUG_ON(fsys_root_fd < 0);

	/* Skip leading slashes.  */
	for (relative_path = path; *relative_path == '/'; relative_path++);

	fd = openat (fsys_root_fd, relative_path, O_RDONLY);
	if (fd == -1)
		return NULL;

	return fdopen(fd, mode);
}

static int ma_accessat(const char *path, int mode) {
	const char *relative_path;

	MA_BUG_ON(fsys_root_fd < 0);

	/* Skip leading slashes.  */
	for (relative_path = path; *relative_path == '/'; relative_path++);

	return faccessat(fsys_root_fd, relative_path, O_RDONLY, 0);
}

static DIR *ma_opendirat(const char *path) {
	int dir_fd;
	const char *relative_path;

	/* Skip leading slashes.  */
	for (relative_path = path; *relative_path == '/'; relative_path++);

	dir_fd = openat(fsys_root_fd, relative_path, O_RDONLY | O_DIRECTORY);
	if (dir_fd < 0)
		return NULL;

	return fdopendir(dir_fd);
}

/* Use our own filesystem functions.  */
#define fopen(p, m)   ma_fopenat(p, m)
#define access(p, m)  ma_accessat(p, m)
#define opendir(p)    ma_opendirat(p)

#else /* !HAVE_OPENAT */

int ma_topology_set_fsys_root(const char *path) {
	mdebug("`ma_topology_set_fsys_root ()' not implemented\n");
	errno = ENOSYS;
	return -1;
}

#endif /* !HAVE_OPENAT */


#ifdef MA__LWPS

static int discovering_level = 1;
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 0;
unsigned marcel_first_cpu = 0;
unsigned marcel_vps_per_cpu = 1;
#  ifdef MA__NUMA
unsigned marcel_topo_max_arity = 4;
#  endif


/** \brief Compute ::marcel_vps_per_cpu, and ::marcel_cpu_stride if not
 *  already set */
static void distribute_vps(void) {
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

static int ma_topo_type_depth[MARCEL_LEVEL_LAST + 1];

int ma_get_topo_type_depth (enum marcel_topo_level_e type) {
  return ma_topo_type_depth[type];
}

marcel_topo_level_t *
ma_topo_lower_ancestor (marcel_topo_level_t *lvl1, marcel_topo_level_t *lvl2) {
  marcel_topo_level_t *l1 = NULL, *l2 = NULL;

  MA_BUG_ON (!lvl1 || !lvl2);
  
  /* There can't be any upper level in that case. */
  if (!lvl1->father)
    return lvl1;
  if (!lvl2->father)
    return lvl2;

  /* We always store the lower level in l1 to simplify the following
     code. */
  l1 = (lvl1->level < lvl2->level) ? lvl1 : lvl2;
  l2 = (lvl1->level < lvl2->level) ? lvl2 : lvl1;

  /* We already know that l2 is the uppered-level topo_level, let's
     lift l1 up to the same level before going any further. */
  while (l1->level < l2->level)
    l1 = l1->father;
    
  /* l1 and l2 should be at the same level now. */
  MA_BUG_ON (l1->level != l2->level);

  /* Maybe l2 is the one we're looking for! (i.e. l1 was originally
     somewhere behind l2, l2 is so the lower ancestor) */
  if (l2->index == l1->index)
    return l2;

  /* If it's not the case, we have to look up until we find fathers
     with the same index (same level + same index = same
     topo_level! */
  while (l2->index != l1->index) {
    MA_BUG_ON (!l1->father || !l2->father);
    l1 = l1->father;
    l2 = l2->father;
  }
  return l2;
}

struct marcel_topo_level *marcel_topo_core_level;
#    undef marcel_topo_node_level
struct marcel_topo_level *marcel_topo_node_level;
static struct marcel_topo_level *marcel_topo_cpu_level;

#    if defined(LINUX_SYS) || defined(SOLARIS_SYS)
static void ma_setup_die_topo_level(int numprocs, unsigned numdies, unsigned *osphysids, unsigned *proc_physids)
{
	struct marcel_topo_level *die_level;
	int j;

	mdebug("%d dies\n", numdies);
	die_level=__marcel_malloc((numdies+MARCEL_NBMAXVPSUP+1)*sizeof(*die_level));
	MA_BUG_ON(!die_level);

	for (j = 0; j < numdies; j++) {
		ma_topo_setup_level(&die_level[j], MARCEL_LEVEL_DIE);
		ma_topo_set_os_numbers(&die_level[j], die, osphysids[j]);
		ma_topo_level_cpuset_from_array(&die_level[j], j, proc_physids, numprocs);
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

static void ma_setup_core_topo_level(int numprocs, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids)
{
	struct marcel_topo_level *core_level;
	int j;

	mdebug("%d cores\n", numcores);
	core_level=__marcel_malloc((numcores+MARCEL_NBMAXVPSUP+1)*sizeof(*core_level));
	MA_BUG_ON(!core_level);

	for (j = 0; j < numcores; j++) {
		ma_topo_setup_level(&core_level[j], MARCEL_LEVEL_CORE);
		ma_topo_set_os_numbers(&core_level[j], core, oscoreids[j]);
		ma_topo_level_cpuset_from_array(&core_level[j], j, proc_coreids, numprocs);
#      ifdef MARCEL_SMT_IDLE
		ma_atomic_init(&core_level[j].nbidle, 0);
#      endif
		mdebug("core %d has cpuset %"MA_VPSET_x" \t(%s)\n",j,core_level[j].cpuset,
				tbx_i2smb(core_level[j].cpuset));
	}

	mdebug("\n");

	marcel_vpset_zero(&core_level[j].vpset);
	marcel_vpset_zero(&core_level[j].cpuset);

	marcel_topo_level_nbitems[discovering_level]=numcores;
	mdebug("--- core level has number %d\n", discovering_level);
	marcel_topo_core_level = marcel_topo_levels[discovering_level++]=core_level;
	mdebug("\n");
}
#endif

#    ifdef LINUX_SYS
#      define PROCESSOR	"processor"
#      define PHYSID		"physical id"
#      define COREID		"core id"
//#  define THREADID	"thread id"

static int ma_parse_sysfs_unsigned(const char *mappath, unsigned *value)
{
	char string[11];
	FILE * fd;

	fd = fopen(mappath, "r");
	if (!fd)
		return -1;

	fgets(string, 11, fd);
	*value = strtol(string, NULL, 10);

	fclose(fd);

	return 0;
}

#define MAX_KERNEL_CPU_MASK 8
#define KERNEL_CPU_MASK_BITS 32
#define KERNEL_CPU_MAP_LEN (KERNEL_CPU_MASK_BITS/4+2)

static int ma_parse_cpumap(const char *mappath, marcel_vpset_t *set)
{
	char string[KERNEL_CPU_MAP_LEN]; /* enough for a shared map mask (32bits hexa) */
	unsigned long maps[MAX_KERNEL_CPU_MASK];
	int nr_maps = 0;
	FILE * fd;
	int i;

	/* reset to zero first */
	marcel_vpset_zero(set);

	fd = fopen(mappath, "r");
	if (!fd)
		return -1;

	/* parse the whole mask */
	while (fgets(string, KERNEL_CPU_MAP_LEN, fd) && *string != '\0') { /* read one kernel cpu mask and the ending comma */
		unsigned long map = strtol(string, NULL, 16);
		if (!map && !nr_maps)
			/* ignore the first map if it's empty */
			continue;
		memmove(&maps[1], &maps[0], (MAX_KERNEL_CPU_MASK-1)*sizeof(*maps));
		maps[0] = map;
		nr_maps++;
	}

	/* check that the map can be stored in our vpset */
	MA_BUG_ON(nr_maps*KERNEL_CPU_MASK_BITS > MARCEL_NBMAXCPUS);

	/* convert into a set */
	for(i=0; i<MAX_KERNEL_CPU_MASK*KERNEL_CPU_MASK_BITS && i<MARCEL_NBMAXCPUS; i++)
		if (maps[i/KERNEL_CPU_MASK_BITS] & 1<<(i%KERNEL_CPU_MASK_BITS))
			marcel_vpset_set(set, i);

	fclose(fd);

	return 0;
}

static void ma_process_cpumap(const char *mappath, const char * mapname, unsigned long val,
		       int nr_procs, unsigned *ids, unsigned long *vals,
		       unsigned *nr_ids, unsigned givenid)
{
	marcel_vpset_t set;
	int k;

	ma_parse_cpumap(mappath, &set);
	for(k=0; k<=nr_procs; k++) {
		if (marcel_vpset_isset(&set, k)) {
			/* we found a cpu in the map */
			unsigned newid;

			if (ids[k] != -1)
				/* already got this map, stop using it */
				break;

			/* allocate a new id, either by incrementing the global counter, or by using the given id */
			newid = nr_ids ? (*nr_ids)++ : givenid;

			/* this cpu didn't have any such id yet, set this id for all cpus in the map */
			for(; k<=nr_procs; k++) {
				if (marcel_vpset_isset(&set, k)) {
					mdebug("--- proc %d has %s number %d\n", k, mapname, newid);
					ids[k] = newid;
					vals[k] = val;
				}
			}

			break;
		}
	}
}

#define CACHE_LEVEL_MAX 3

static void
ma_parse_cache_shared_cpu_maps(int proc_index, int nr_procs, unsigned *cacheids, unsigned long *cachesizes, unsigned *nr_caches)
{
	int i;

	for(i=0; i<10; i++) {
#define SHARED_CPU_MAP_STRLEN (27+9+12+1+15+1)
		char mappath[SHARED_CPU_MAP_STRLEN];
		char string[20]; /* enough for a level number (one digit) or a type (Data/Instruction/Unified) */
		char cachename[8+1];
		unsigned long kB = 0;
		int level; /* 0 for L1, .... */
		FILE * fd;

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
				fclose(fd);
				if (!strncmp(string,"Instruction", 11))
					continue;
			} else {
				fclose(fd);
				continue;
			}
		} else
			continue;

		sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/size", proc_index, i);
		fd = fopen(mappath, "r");
		if (fd) {
			if (fgets(string,sizeof(string), fd))
				kB = atol(string); /* in kB */
			fclose(fd);
		}

		sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", proc_index, i);
		sprintf(cachename, "L%d cache", level+1);
		ma_process_cpumap(mappath, cachename, kB,
				  nr_procs, cacheids+level*MARCEL_NBMAXCPUS,
				  cachesizes+level*MARCEL_NBMAXCPUS,
				  &nr_caches[level], -1);
	}
}

static void
ma_setup_cache_topo_level(int cachelevel, enum marcel_topo_level_e topotype, int nr_procs,
			  unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes)
{
	struct marcel_topo_level *level;
	int j;

	mdebug("%d L%d caches\n", cachelevel+1, numcaches[cachelevel]);
	level=__marcel_malloc((numcaches[cachelevel]+MARCEL_NBMAXVPSUP+1)*sizeof(*level));
	MA_BUG_ON(!level);

	for (j = 0; j < numcaches[cachelevel]; j++) {
		ma_topo_setup_level(&level[j], topotype);

		switch (cachelevel) {
		case 2: ma_topo_set_os_numbers(&level[j], l3, j); break;
		case 1: ma_topo_set_os_numbers(&level[j], l2, j); break;
		case 0: ma_topo_set_os_numbers(&level[j], l1, j); break;
		default: MA_BUG_ON(1);
		}

		level[j].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L1+cachelevel] = cachesizes[cachelevel*MARCEL_NBMAXCPUS+j];

		ma_topo_level_cpuset_from_array(&level[j], j, &cacheids[cachelevel*MARCEL_NBMAXCPUS], nr_procs);

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

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void look__sysfscpu(unsigned *nr_procs,
			   unsigned *nr_cores,
			   unsigned *nr_dies,
			   unsigned *proc_physids,
			   unsigned *osphysids,
			   unsigned *proc_coreids,
			   unsigned *oscoreids
) {
#define CPU_TOPOLOGY_STR_LEN (27+9+29+1)
	char string[CPU_TOPOLOGY_STR_LEN];
	int i,j,k;

	for(i=0; ; i++) {
		marcel_vpset_t dieset, coreset;
		unsigned mydieid, mycoreid;

		sprintf(string, "/sys/devices/system/cpu/cpu%d/topology", i);

		if (access(string, R_OK) < 0
		    && errno == ENOENT)
			break;

		mydieid = 0; /* shut-up the compiler */
		sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
		ma_parse_sysfs_unsigned(string, &mydieid);

		mycoreid = 0; /* shut-up the compiler */
		sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_id", i);
		ma_parse_sysfs_unsigned(string, &mycoreid);

		sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_siblings", i);
		ma_parse_cpumap(string, &dieset);
		/* make sure we are in the set, in case the cpumap was crap */
		marcel_vpset_set(&dieset, i);

		sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", i);
		ma_parse_cpumap(string, &coreset);
		/* make sure we are in the set, in case the cpumap was crap */
		marcel_vpset_set(&coreset, i);

		for(j=0; j<i; j++)
			if (marcel_vpset_isset(&dieset, j))
				break;
		if (j==i) {
			/* new die cpumap fill it */
			for(k=i; k<MARCEL_NBMAXCPUS; k++)
				if (marcel_vpset_isset(&dieset, k))
					proc_physids[k] = (*nr_dies);
			mdebug("die %d (os %d) has cpuset %"MA_VPSET_x" \t(%s)\n",
			       (*nr_dies), mydieid, dieset, tbx_i2smb(dieset));
			osphysids[(*nr_dies)++] = mydieid;
		}

		for(j=0; j<i; j++)
			if (marcel_vpset_isset(&coreset, j))
				break;
		if (j==i) {
			/* new core cpumap fill it */
			for(k=i; k<MARCEL_NBMAXCPUS; k++)
				if (marcel_vpset_isset(&coreset, k))
					proc_coreids[k] = (*nr_cores);
			mdebug("core %d (os %d) has cpuset %"MA_VPSET_x" \t(%s)\n",
			       (*nr_cores), mycoreid, coreset, tbx_i2smb(coreset));
			oscoreids[(*nr_cores)++] = mycoreid;
		}
	}

	*nr_procs = i;
	mdebug("%s: found %u procs\n", __func__, *nr_procs);
}

/* Look at Linux' /proc/cpuinfo */
static void look_cpuinfo(unsigned *nr_procs,
			 unsigned *nr_cores,
			 unsigned *nr_dies,
			 unsigned *proc_physids,
			 unsigned *osphysids,
			 unsigned *proc_coreids,
			 unsigned *oscoreids
) {
	FILE *fd;
	char string[strlen(PHYSID)+1+9+1+1];
	char *endptr;
	unsigned proc_osphysids[MARCEL_NBMAXCPUS];
	unsigned proc_oscoreids[MARCEL_NBMAXCPUS];
	unsigned core_osphysids[MARCEL_NBMAXCPUS];
	long physid;
	long coreid;
	long processor = -1;
	int i;

	memset(proc_physids,0,sizeof(proc_physids));
	memset(proc_osphysids,0,sizeof(proc_osphysids));
	memset(proc_coreids,0,sizeof(proc_coreids));
	memset(proc_oscoreids,0,sizeof(proc_oscoreids));

	if (!(fd=fopen("/proc/cpuinfo","r"))) {
		fprintf(stderr,"could not open /proc/cpuinfo\n");
		return;
	}

	/* Just record information and count number of dies and cores */

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
			} else if (var==LONG_MAX) { \
				fprintf(stderr,"too big "field" number in /proc/cpuinfo\n"); \
				break; \
			} \
			mdebug(field " %ld\n", var)
#      define getprocnb_end() \
		}
		getprocnb_begin(PROCESSOR,processor);
		getprocnb_end() else
		getprocnb_begin(PHYSID,physid);
			proc_osphysids[processor]=physid;
			for (i=0; i<*nr_dies; i++)
				if (physid == osphysids[i])
					break;
			proc_physids[processor]=i;
			mdebug("%ld on die %d (%lx)\n", processor, i, physid);
			if (i==*nr_dies)
				osphysids[(*nr_dies)++] = physid;
		getprocnb_end() else
		getprocnb_begin(COREID,coreid);
			proc_oscoreids[processor]=coreid;
			for (i=0; i<*nr_cores; i++)
				if (coreid == oscoreids[i] && proc_osphysids[processor] == core_osphysids[i])
					break;
			proc_coreids[processor]=i;
			mdebug("%ld on core %d (%lx:%x)\n", processor, i, coreid, proc_osphysids[processor]);
			if (i==*nr_cores) {
				core_osphysids[*nr_cores] = proc_osphysids[processor];
				oscoreids[*nr_cores] = coreid;
				(*nr_cores)++;
			}
		getprocnb_end()
		if (string[strlen(string)-1]!='\n') {
			fscanf(fd,"%*[^\n]");
			getc(fd);
		}
	}
	fclose(fd);

	/* setup the final number of procs */
	*nr_procs = processor + 1;
	mdebug("%s: found %u procs\n", __func__, *nr_procs);
}

static void __marcel_init look_sysfscpu(void) {
	unsigned proc_physids[MARCEL_NBMAXCPUS];
	unsigned osphysids[MARCEL_NBMAXCPUS];
	unsigned proc_coreids[MARCEL_NBMAXCPUS];
	unsigned oscoreids[MARCEL_NBMAXCPUS];
	unsigned proc_cacheids[CACHE_LEVEL_MAX*MARCEL_NBMAXCPUS];
	unsigned long proc_cachesizes[CACHE_LEVEL_MAX*MARCEL_NBMAXCPUS];
	int j,k;

	unsigned numprocs=0;
	unsigned numdies=0;
	unsigned numcores=0;
	unsigned numcaches[CACHE_LEVEL_MAX];

	if (access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK) < 0
	    || access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK) < 0
	    || access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK) < 0
	    || access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK) < 0) {
	  /* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
	  look_cpuinfo(&numprocs, &numcores, &numdies,
		       proc_physids, osphysids,
		       proc_coreids, oscoreids);
	} else {
	  look__sysfscpu(&numprocs, &numcores, &numdies,
		       proc_physids, osphysids,
		       proc_coreids, oscoreids);
	}

	mdebug("\n\n * Topology summary *\n\n");
	mdebug("%d processors\n", numprocs);

	if (numdies>1)
		mdebug("%d dies\n", numdies);
	if (numcores>1)
		mdebug("%d cores\n", numcores);

	mdebug("\n\n * cpusets details *\n\n");
	if (numdies>1)
		ma_setup_die_topo_level(numprocs, numdies, osphysids, proc_physids);

	for(j=0; j<CACHE_LEVEL_MAX; j++) {
		numcaches[j] = 0;
		for(k=0; k<numprocs; k++)
			proc_cacheids[j*MARCEL_NBMAXCPUS+k] = -1;
	}
	for(j=0; j<numprocs; j++) {
		ma_parse_cache_shared_cpu_maps(j, numprocs, proc_cacheids, proc_cachesizes, numcaches);
	}

	if (numcaches[2] > 0) {
		/* setup L3 caches */
		ma_setup_cache_topo_level(2, MARCEL_LEVEL_L3, numprocs, numcaches, proc_cacheids, proc_cachesizes);
	}

	if (numcaches[1] > 0) {
		/* setup L2 caches */
		ma_setup_cache_topo_level(1, MARCEL_LEVEL_L2, numprocs, numcaches, proc_cacheids, proc_cachesizes);
	}

	if (numcores>1)
		ma_setup_core_topo_level(numprocs, numcores, oscoreids, proc_coreids);

	if (numcaches[0] > 0) {
		/* setup L1 caches */
		ma_setup_cache_topo_level(0, MARCEL_LEVEL_L1, numprocs, numcaches, proc_cacheids, proc_cachesizes);
	}

	/* Override the default returned by `ma_fallback_nbprocessors ()'.  */
	marcel_nbprocessors = numprocs;
	mdebug("%s: found %u procs\n", __func__, marcel_nbprocessors);
}

static unsigned long ma_sysfs_node_meminfo_to_hugepagefree(const char * path)
{
	char string[64];
	FILE *fd;

	fd = fopen(path, "r");
	if (!fd)
		return 0;

	while (fgets(string, sizeof(string), fd) && *string != '\0') {
		int node;
		unsigned long hugepagefree;
		if (sscanf(string, "Node %d HugePages_Free: %ld kB", &node, &hugepagefree) == 2) {
			fclose(fd);
			return hugepagefree;
		}
	}

	fclose(fd);
	return 0;
}

static unsigned long ma_sysfs_node_meminfo_to_memsize(const char * path)
{
	char string[64];
	FILE *fd;

	fd = fopen(path, "r");
	if (!fd)
		return 0;

	while (fgets(string, sizeof(string), fd) && *string != '\0') {
		int node;
		unsigned long size;
		if (sscanf(string, "Node %d MemTotal: %ld kB", &node, &size) == 2) {
			fclose(fd);
			return size;
		}
	}

	fclose(fd);
	return 0;
}

static void __marcel_init look_sysfsnode(void) {
	unsigned i;
	unsigned nbnodes = 1;
	struct marcel_topo_level *node_level;
	DIR *dir;
	struct dirent *dirent;

	dir = opendir("/sys/devices/system/node");
	if (dir) {
		while ((dirent = readdir(dir)) != NULL) {
			unsigned long node;
			if (strncmp(dirent->d_name, "node", 4))
				continue;
			node = strtoul(dirent->d_name+4, NULL, 0);
			if (nbnodes < node+1)
				nbnodes = node+1;
		}
		closedir(dir);
	}

	if (nbnodes <= 1) {
		ma_numa_not_available=1;
		return;
	}

	node_level=__marcel_malloc((nbnodes+MARCEL_NBMAXVPSUP+1)*sizeof(*node_level));
	MA_BUG_ON(!node_level);

	for (i=0;;i++) {
#define NUMA_NODE_STRLEN (29+9+8+1)
		char nodepath[NUMA_NODE_STRLEN];
		marcel_vpset_t cpuset;
		unsigned long size;
                unsigned long hpfree;
		int j;

		sprintf(nodepath, "/sys/devices/system/node/node%d/cpumap", i);
		if (ma_parse_cpumap(nodepath, &cpuset) < 0)
			break;

		sprintf(nodepath, "/sys/devices/system/node/node%d/meminfo", i);
		size = ma_sysfs_node_meminfo_to_memsize(nodepath);
		hpfree = ma_sysfs_node_meminfo_to_hugepagefree(nodepath);

		ma_topo_setup_level(&node_level[i], MARCEL_LEVEL_NODE);
		ma_topo_set_os_numbers(&node_level[i], node, i);
		node_level[i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE] = size;
                node_level[i].huge_page_free = hpfree;

		node_level[i].cpuset = cpuset;
		for(j=0;j<MARCEL_NBMAXCPUS;j++)
			if (marcel_vpset_isset(&cpuset, j))
				ma_vp_node[j] = i;

		mdebug("node %d has cpuset %"MA_VPSET_x"\n", i, node_level[i].vpset);
	}
	nbnodes = i;

	marcel_vpset_zero(&node_level[nbnodes].vpset);
	marcel_vpset_zero(&node_level[nbnodes].cpuset);

	marcel_topo_level_nbitems[discovering_level] = marcel_nbnodes = nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;
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

		ma_topo_setup_level(&node_level[i], MARCEL_LEVEL_NODE);
		ma_topo_set_os_numbers(&node_level[i], node, radid);
		node_level[i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE] = 0; /* unknown */
                node_level[i].huge_page_free = 0;

		cursor = SET_CURSOR_INIT;
		while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
			marcel_vpset_set(&node_level[i].cpuset,cpuid);

		mdebug("node %d has cpuset %"MA_VPSET_x"\n",i,node_level[i].cpuset);
		i++;
	}

	marcel_vpset_zero(&node_level[i].vpset);
	marcel_vpset_zero(&node_level[i].cpuset);

	marcel_topo_level_nbitems[discovering_level] = marcel_nbnodes = nbnodes;
	marcel_topo_levels[discovering_level++] =
		marcel_topo_node_level = node_level;
}
#    endif /* OSF_SYS */

#    ifdef SOLARIS_SYS
#      include <sys/lgrp_user.h>
static void show(lgrp_cookie_t cookie, lgrp_id_t lgrp) {
	processorid_t cpuids[32];
	lgrp_id_t lgrps[32];
	int i, n;
	n = lgrp_cpus(cookie, lgrp, cpuids, 32, LGRP_CONTENT_ALL);
	for (i = 0; i < n ; i++) {
		mdebug("%ld has %d\n", lgrp, cpuids[i]);
	}
	n = lgrp_children(cookie, lgrp, lgrps, 32);
	mdebug("%ld %d children\n", lgrp, n);
	for (i = 0; i < n ; i++) {
		show(cookie, lgrps[i]);
	}
	mdebug("%ld children done\n", lgrp);
}

static void __marcel_init look_lgrp(void) {
	lgrp_cookie_t cookie = lgrp_init(LGRP_VIEW_OS);
	lgrp_id_t root;
	if (cookie == LGRP_COOKIE_NONE) {
		mdebug("lgrp_init failed: %s\n", strerror(errno));
		return;
	}
	root = lgrp_root(cookie);
	show(cookie, root);
	/* TODO */
	lgrp_fini(cookie);
}

#include <kstat.h>
static void __marcel_init look_kstat(void) {
	kstat_ctl_t *kc = kstat_open();
	kstat_t *ksp;
	kstat_named_t *stat;
	unsigned proc_physids[MARCEL_NBMAXCPUS];
	unsigned proc_osphysids[MARCEL_NBMAXCPUS];
	unsigned osphysids[MARCEL_NBMAXCPUS];
	unsigned proc_coreids[MARCEL_NBMAXCPUS];
	unsigned proc_oscoreids[MARCEL_NBMAXCPUS];
	unsigned oscoreids[MARCEL_NBMAXCPUS];
	unsigned core_osphysids[MARCEL_NBMAXCPUS];
	unsigned physid, coreid;
	unsigned numprocs = 0;
	unsigned numdies = 0;
	unsigned numcores = 0;
	unsigned i;

	if (!kc) {
		mdebug("kstat_open failed: %s\n", strerror(errno));
		return;
	}
	for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
		if (strncmp("cpu_info", ksp->ks_module, 8))
			continue;

		if (ksp->ks_instance != numprocs) {
			fprintf(stderr, "kstat instances not in CPU order: %d comes %d\n", ksp->ks_instance, numprocs);
			goto out;
		}
		if (kstat_read(kc, ksp, NULL) == -1) {
			fprintf(stderr, "kstat_read failed for CPU%d: %s\n", numprocs, strerror(errno));
			goto out;
		}
		stat = (kstat_named_t *) kstat_data_lookup(ksp, "chip_id");
		if (!stat) {
			if (numdies) {
				fprintf(stderr, "could not read die id for CPU%d: %s\n", numprocs, strerror(errno));
				goto out;
			}
		} else {
			if (stat->data_type != KSTAT_DATA_INT32) {
				fprintf(stderr, "chip_id is not an INT32\n");
				goto out;
			}
			proc_osphysids[numprocs] = physid = stat->value.i32;
			for (i = 0; i < numdies; i++)
				if (physid == osphysids[i])
					break;
			proc_physids[numprocs] = i;
			mdebug("%u on die %u (%u)\n", numprocs, i, physid);
			if (i == numdies)
				osphysids[numdies++] = physid;
		}

		stat = (kstat_named_t *) kstat_data_lookup(ksp, "core_id");
		if (!stat) {
			if (numcores) {
				fprintf(stderr, "could not read core id for CPU%d: %s\n", numprocs, strerror(errno));
				goto out;
			}
		} else {
			if (stat->data_type != KSTAT_DATA_INT32) {
				fprintf(stderr, "core_id is not an INT32\n");
				goto out;
			}
			proc_oscoreids[numprocs] = coreid = stat->value.i32;
			for (i = 0; i < numcores; i++)
				if (coreid == oscoreids[i] && proc_osphysids[numprocs] == core_osphysids[i])
					break;
			proc_coreids[numprocs] = i;
			mdebug("%u on core %u (%u)\n", numprocs, i, coreid);
			if (i == numcores) {
				core_osphysids[numcores] = proc_osphysids[numprocs];
				oscoreids[numcores++] = coreid;
			}
		}

		numprocs++;
	}

	if (numdies > 1)
		ma_setup_die_topo_level(numprocs, numdies, osphysids, proc_physids);

	if (numcores > 1)
		ma_setup_core_topo_level(numprocs, numcores, oscoreids, proc_coreids);
out:
	kstat_close(kc);
}
#    endif /* SOLARIS_SYS */

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
		ma_topo_set_empty_os_numbers(&rad_level[r]);
		switch(level) {
			case MARCEL_LEVEL_NODE:
				rad_level[r].os_node = r;
				rad_level[r].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE] = 0; /* unknown */
                                rad_level[r].huge_page_free = 0;
				break;
			case MARCEL_LEVEL_L2:
				rad_level[r].os_l2 = r;
				rad_level[r].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L2] = 0; /* unknown */
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
			ma_atomic_init(&rad_level[r].nbidle, 0);
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
	if (level == MARCEL_LEVEL_CORE)
		marcel_topo_core_level = rad_level;
	rs_free(rset);
	rs_free(rad);
}
#    endif /* AIX_SYS */

#    ifdef WIN_SYS
#      warning TODO: use GetLogicalProcessorInformation, GetNumaHighestNodeNumber, and GetNumaNodeProcessorMask
#    endif

static unsigned *
ma_split_quirk(enum marcel_topo_level_e ptype, unsigned arity, enum marcel_topo_level_e ctype,
	       unsigned *subarity, unsigned *sublevels)
{
	unsigned * array = NULL;
#ifdef MA__NUMA
	if (0 && /* disabled for now */
	    ptype == MARCEL_LEVEL_MACHINE && ctype == MARCEL_LEVEL_NODE) {
		/* splitting machine->numanode arity */

#ifdef LINUX_SYS
		/* get DMI board vendor and name */
#define DMI_BOARD_STRINGS_LEN 50
		char vendor[DMI_BOARD_STRINGS_LEN], device[DMI_BOARD_STRINGS_LEN];
		FILE *fd;

		vendor[0]='\0';
		fd = fopen("/sys/class/dmi/id/board_vendor", "r");
		if (fd) {
			fgets(vendor, DMI_BOARD_STRINGS_LEN, fd);
			fclose (fd);
		}

		device[0]='\0';
		fd = fopen("/sys/class/dmi/id/board_name", "r");
		if (fd) {
			fgets(device, DMI_BOARD_STRINGS_LEN, fd);
			fclose (fd);
		}

		/* quirk for tyan 8-opteron machines */
		if (arity == 8
		    && !strncasecmp(vendor, "tyan", 4)
		    && (!strncasecmp(device, "S4881", 5)
			|| !strncasecmp(device, "S4885", 5)
			|| !strncasecmp(device, "S4985", 5))) {

			mdebug("splitting machine->numanode on Tyan 8 opteron board, apply split quirk\n");

			array = malloc(8*sizeof(*array));
			MA_BUG_ON(!array);

			/* create 4 fake levels with 2 nodes that are
			 * actually connected through a single link
			 */
			*subarity = 2;
			*sublevels = 4;
		
			array[0] = 0;
			array[1] = 0;
			array[2] = 1;
			array[3] = 1;
			array[4] = 2;
			array[5] = 3;
			array[6] = 3;
			array[7] = 2;
		}
#endif /* LINUX_SYS */
	}
#endif /* MA_NUMA */

	return array;
}

/* Use the value stored in marcel_nbprocessors.  */
static void look_cpu(void) {
	struct marcel_topo_level *cpu_level;
	unsigned cpu;

	cpu_level=__marcel_malloc((marcel_nbprocessors+MARCEL_NBMAXVPSUP+1)*sizeof(*cpu_level));
	MA_BUG_ON(!cpu_level);

	mdebug("\n\n * CPU cpusets *\n\n");
	for (cpu=0; cpu<marcel_nbprocessors; cpu++) {
		ma_topo_setup_level(&cpu_level[cpu], MARCEL_LEVEL_PROC);
		ma_topo_set_os_numbers(&cpu_level[cpu], cpu, cpu);

		marcel_vpset_vp(&cpu_level[cpu].cpuset, cpu);

		mdebug("cpu %d has cpuset %"MA_VPSET_x" \t(%s)\n",cpu,cpu_level[cpu].cpuset, tbx_i2smb(cpu_level[cpu].cpuset));
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
	if(!arity){
		*sublevelarity = *nbsublevels = 0;
		return;
	}
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

	/* Initialize the number of processor to some reasonable default, e.g.,
		 obtained using sysconf(3).  */
	marcel_nbprocessors = ma_fallback_nbprocessors ();

#ifdef HAVE_OPENAT
	if (fsys_root_fd < 0) {
		/* Get a file descriptor to the file system root.  */
		if (ma_topology_set_fsys_root("/")) {
			perror ("opening file system root");
			MA_BUG();
		}
	}
#endif

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
	look_sysfsnode();
	look_sysfscpu();
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
#    ifdef SOLARIS_SYS
	look_lgrp();
	look_kstat();
#    endif /* SOLARIS_SYS */
	look_cpu();
#  endif /* MA__NUMA */

	marcel_topo_nblevels=discovering_level;
	mdebug("\n\n--> discovered %d levels\n\n", marcel_topo_nblevels);

	/* Set the number of VPs, unless already specified.  */
	if (ma__nb_vp == 0)
		ma__nb_vp = marcel_nbprocessors;

	mdebug("%s: chose %u VPs\n", __func__, ma__nb_vp);

	distribute_vps();

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
		ma_topo_setup_level(&vp_level[i], MARCEL_LEVEL_VP);
		ma_topo_set_os_numbers(&vp_level[i], cpu, oscpu);

		marcel_vpset_vp(&vp_level[i].cpuset, oscpu);
		marcel_vpset_set(&cpuset, oscpu);

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
	/* merge identical levels */
	for (l=0; l+1<marcel_topo_nblevels; l++) {
		for (i=0; marcel_topo_levels[l][i].cpuset; i++);
		for (j=0; j<i && marcel_topo_levels[l+1][j].cpuset; j++)
			if (marcel_topo_levels[l+1][j].cpuset != marcel_topo_levels[l][j].cpuset)
				break;
		if (j==i && !marcel_topo_levels[l+1][j].cpuset) {
			mdebug("merging levels %u and %u since same %d item%s\n", l, l+1, i, i>=2?"s":"");
			if (marcel_topo_levels[l+1] == marcel_topo_cpu_level)
				marcel_topo_cpu_level = marcel_topo_levels[l];
			else if (marcel_topo_levels[l+1] == marcel_topo_core_level)
				marcel_topo_core_level = marcel_topo_levels[l];
			else if (marcel_topo_levels[l+1] == marcel_topo_node_level)
				marcel_topo_node_level = marcel_topo_levels[l];
			for (i=0; i<marcel_topo_level_nbitems[l]; i++) {
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

#    define merge_memory_kB(_type) \
				if (marcel_topo_levels[l+1][i].merged_type & (1<<MARCEL_LEVEL_##_type)) { \
					MA_BUG_ON(marcel_topo_levels[l][i].merged_type & (1<<MARCEL_LEVEL_##_type)); \
					marcel_topo_levels[l][i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_##_type] \
						= marcel_topo_levels[l+1][i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_##_type]; \
				}
				merge_memory_kB(L1);
				merge_memory_kB(L2);
				merge_memory_kB(L3);
				merge_memory_kB(NODE);

				/* merge types at the end so that the above assertions work */
				marcel_topo_levels[l][i].merged_type |= marcel_topo_levels[l+1][i].merged_type;
			}
			__marcel_free(marcel_topo_levels[l+1]);
			memmove(&marcel_topo_level_nbitems[l+1],&marcel_topo_level_nbitems[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_level_nbitems));
			memmove(&marcel_topo_levels[l+1],&marcel_topo_levels[l+2],(marcel_topo_nblevels-(l+2))*sizeof(*marcel_topo_levels));
			marcel_topo_nblevels--;
			marcel_topo_levels[marcel_topo_nblevels] = NULL;
			l--;
		}
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

					unsigned * quirk_array;
					quirk_array = ma_split_quirk(marcel_topo_levels[l][i].type, marcel_topo_levels[l][i].arity, marcel_topo_levels[l+2][0].type,
								     &sublevelarity, &nbsublevels);
					if (!quirk_array)
						split(marcel_topo_levels[l][i].arity,&sublevelarity,&nbsublevels);

					mdebug("splitting level %u,%u into %u sublevels of size at most %u\n", l, i, nbsublevels, sublevelarity);
					/* initialize subitems */
					for (k=0; k<nbsublevels; k++) {
						level = &marcel_topo_levels[l+1][j+k];
						level->type = MARCEL_LEVEL_FAKE;
						level->merged_type = 1<<MARCEL_LEVEL_FAKE;
						level->number = j+k;
						level->index = k;
						ma_topo_set_empty_os_numbers(level);
						marcel_vpset_zero(&level->cpuset);
						marcel_vpset_zero(&level->vpset);
						level->arity = 0;
						level->children = TBX_MALLOC(sublevelarity*sizeof(void*));
						level->father = &marcel_topo_levels[l][i];
					}

					/* distribute cpus to subitems */
					/* give cpus of sublevelarity items to each fake item */

					if (quirk_array) {
						/* use the quirk array to connect new levels */
						for (n=0; n<marcel_topo_levels[l][i].arity; n++) {
							unsigned quirk_father = quirk_array[n];
							unsigned child_index;
							level = &marcel_topo_levels[l+1][j+quirk_father];
							level->cpuset |=
								marcel_topo_levels[l][i].children[n]->cpuset;
							child_index = level->arity;
							level->children[child_index] = marcel_topo_levels[l][i].children[n];
							marcel_topo_levels[l][i].children[n]->index = child_index;
							marcel_topo_levels[l][i].children[n]->father = level;
							level->arity++;
							MA_BUG_ON(level->arity > sublevelarity);
						}
						free(quirk_array);

					} else {
						/* linear split */

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
					}

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

	/* intialize all depth to unknown */
	for (l=0; l <= MARCEL_LEVEL_LAST; l++)
		ma_topo_type_depth[l] = -1;

	/* walk the existing levels to set their depth */
	for (l=0; l<marcel_topo_nblevels; l++)
		ma_topo_type_depth[marcel_topo_levels[l][0].type] = l;

	/* setup the depth of all still unknown levels (the one that got merged or never created */
	int type, prevdepth = -1;
	for (type = 0; type <= MARCEL_LEVEL_LAST; type++) {
	  if (ma_topo_type_depth[type] == -1)
	    ma_topo_type_depth[type] = prevdepth;
	  else
	    prevdepth = ma_topo_type_depth[type];
	}
#  endif /* MA__NUMA */

	MA_BUG_ON(marcel_topo_level_nbitems[marcel_topo_nblevels-1] != marcel_nbvps());
	marcel_topo_vp_level = marcel_topo_levels[marcel_topo_nblevels-1];

	/* Set VP sets */
	/* This is the only one which isn't empty by default */
	marcel_vpset_zero(&marcel_machine_level[0].vpset);
	for (i=0; i<marcel_nbvps(); i++) {
		level = &marcel_topo_vp_level[i];
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
		ma_topo_set_empty_os_numbers(&marcel_topo_vp_level[i]);
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


#ifdef MA__NUMA

/* Synthetic or "fake" topology creation, for testing purposes.

	 Uses the `synth_' name space, except for global variables.  */


/* Allocate COUNT nodes of type TYPE as children of LEVEL, with numbers
	 starting from FIRST_NUMBER.  Individual nodes are allocated from
	 NODE_POOL, which must point to an array of at least COUNT items (children
	 N's storage will be NODE_POOL[N]).  */
static void
synth_make_children(struct marcel_topo_level *level, unsigned count,
										enum marcel_topo_level_e type, unsigned first_number,
										struct marcel_topo_level *node_pool)
{
	unsigned i;

	level->children = TBX_CALLOC(count, sizeof(*level->children));
	MA_BUG_ON(level->children == NULL);

	for (i = 0; i < count; i++) {
		level->children[i] = &node_pool[i];
		MA_BUG_ON(level->children[i] == NULL);

		level->arity = count;

		level->children[i]->father = level;
		level->children[i]->index = i;
		level->children[i]->children = NULL;
		level->children[i]->arity = 0;
		level->children[i]->level = level->level + 1;
		level->children[i]->number = first_number + i;
		level->children[i]->type = type;
		level->children[i]->merged_type = 1 << (int) type;

		switch(type) {
		case MARCEL_LEVEL_PROC:
			ma_topo_set_os_numbers(level->children[i], cpu, i);
			break;
		case MARCEL_LEVEL_CORE:
			ma_topo_set_os_numbers(level->children[i], core, i);
			break;
		case MARCEL_LEVEL_DIE:
			ma_topo_set_os_numbers(level->children[i], die, i);
			break;
		case MARCEL_LEVEL_NODE:
			ma_topo_set_os_numbers(level->children[i], node, i);
			break;
		default:
			ma_topo_set_empty_os_numbers(level->children[i]);
		}
	}
}

/* Determine a level type for for the level whose description is pointed to
	 by LEVEL_BREADTH.  */
static enum marcel_topo_level_e
synth_level_type (const unsigned *level_breadth)
{
	return (*(level_breadth + 1) == 0
					? MARCEL_LEVEL_PROC
					: (*(level_breadth + 2) == 0
						 ? MARCEL_LEVEL_CORE
						 : (*(level_breadth + 3) == 0
								? MARCEL_LEVEL_DIE
								: (*(level_breadth + 4) == 0
									 ? MARCEL_LEVEL_NODE
									 : MARCEL_LEVEL_FAKE))));
}

/* Recursively populate the topology starting from LEVEL according to
   LEVEL_BREADTH, and number VPs starting at FIRST_VP.  Return the total
   number of VPs beneath LEVEL.  */
static unsigned
synth_populate_topology(struct marcel_topo_level *level,
												const unsigned *level_breadth, unsigned first_vp)
{
	unsigned i, vp_count;
	enum marcel_topo_level_e type;

	/* Increment the total breadth for this level.  */
	marcel_topo_level_nbitems[level->level]++;

	/* Recursion ends when *LEVEL_BREADTH is zero, meaning that LEVEL has no
	   children.  */
	if (*level_breadth > 0) {
		unsigned siblings;

		/* Processors don't have children.  */
		MA_BUG_ON(level->type == MARCEL_LEVEL_PROC);

		/* Determine the children level type.  */
		type = synth_level_type (level_breadth);

		/* Current number of siblings on our children's level to our left.  */
		siblings = marcel_topo_level_nbitems[level->level + 1];

		synth_make_children(level, *level_breadth, type, siblings,
												&marcel_topo_levels[level->level + 1][siblings]);

		for (i = 0, vp_count = 0; i < *level_breadth; i++)
			vp_count += synth_populate_topology(level->children[i],
																					level_breadth + 1,
																					first_vp + vp_count);

		/* Aggregate the VP sets of our kids.  */
		marcel_vpset_zero(&level->vpset);
		for (i = 0; i < *level_breadth; i++)
			level->vpset |= level->children[i]->vpset;

		/* XXX: We don't pay attention to the CPU set as it doesn't seem to be
			 used outside of `marcel_topology.c'; we just initialize it so that it's
			 non-zero.  */
		level->cpuset = level->vpset;
	} else {
		/* Only processors have no children.  */
		MA_BUG_ON(level->type != MARCEL_LEVEL_PROC);

		level->merged_type |= 1 << MARCEL_LEVEL_VP;

		marcel_vpset_zero(&level->vpset);
		marcel_vpset_set(&level->vpset, first_vp);
		level->cpuset = level->vpset;

		vp_count = 1;
	}

	return vp_count;
}

/* Allocate an array for each topology level described by TOPOLOGY.  These
	 arrays are stored in the global `marcel_topo_levels' variable.  */
static void
synth_allocate_topology_levels(const unsigned *topology) {
	const unsigned *level_breadth;
	unsigned level, total_level_breadth;

	marcel_topo_nblevels = 1;
	marcel_topo_levels[0] = marcel_machine_level;

	for (level = 1, level_breadth = topology, total_level_breadth = 1;
			 *level_breadth > 0;
			 level++, level_breadth++) {
		unsigned count;

		total_level_breadth *= *level_breadth;

		count = total_level_breadth + 1;
		if (*(level_breadth + 1) == 0)
			/* Things like `for_all_vp ()' except that many elements.  */
			count += MARCEL_NBMAXVPSUP;

		mdebug("synthetic topology: creating level %u with breadth %u (%u children per father)\n",
					 marcel_topo_nblevels, total_level_breadth, *level_breadth);
		marcel_topo_levels[level] =
			__marcel_malloc(count * sizeof(marcel_topo_levels[0][0]));

		MA_BUG_ON(marcel_topo_levels[level] == NULL);

		/* Each level is terminated by an item with zeroed VP sets.  */
		marcel_topo_levels[level][total_level_breadth].vpset = 0;
		marcel_topo_levels[level][total_level_breadth].cpuset = 0;

		/* Update the level type to level mapping.  */
		ma_topo_type_depth[synth_level_type (level_breadth)] = level;
		if (synth_level_type (level_breadth) == MARCEL_LEVEL_PROC)
			/* We don't have a separate VP level, but the proc level is
				 conceptually also the VP level.  */
			ma_topo_type_depth[MARCEL_LEVEL_VP] = level;

		marcel_topo_nblevels++;
	}

	MA_BUG_ON(marcel_topo_nblevels <= 1);

	mdebug("synthetic topology: total number of levels: %u\n",
				 marcel_topo_nblevels);

	/* The leaves of the topology tree are assumed to be VPs.  */
	marcel_topo_vp_level = marcel_topo_levels[level - 1];
}

static struct marcel_topo_level *
synth_make_simple_topology(const unsigned *topology_description) {
	unsigned level;
	struct marcel_topo_level *root, *vp;

	/* Initialize level breadth and depth.  */
	for (level = 0; level <= MARCEL_LEVEL_LAST; level++) {
		marcel_topo_level_nbitems[level] = 0;
		ma_topo_type_depth[level] = -1;
	}
	ma_topo_type_depth[MARCEL_LEVEL_MACHINE] = 0;

	synth_allocate_topology_levels(topology_description);

	root = &marcel_machine_level[0];

	root->father = NULL;
	root->index = 0;
	root->level = 0;
	root->type = MARCEL_LEVEL_MACHINE;
	root->merged_type = 1 << (int) root->type;

	root->os_cpu =
			root->os_die =
			root->os_l3 =
			root->os_l2 = root->os_l1 = root->os_core = root->os_node = -1;

	synth_populate_topology(root, topology_description, 0);
	MA_BUG_ON(root->arity != *topology_description);

	/* Set the total number of VPs and processors.
	   FIXME: We currently ignore user settings via `--marcel-nvp'.  */
	MA_BUG_ON (ma_topo_type_depth[MARCEL_LEVEL_VP] == -1);
	ma__nb_vp = marcel_topo_level_nbitems[ma_topo_type_depth[MARCEL_LEVEL_VP]];

	MA_BUG_ON (ma_topo_type_depth[MARCEL_LEVEL_PROC] == -1);
	marcel_nbprocessors = marcel_topo_level_nbitems[ma_topo_type_depth[MARCEL_LEVEL_PROC]];

	mdebug("%s: %u processors, chose %u VPs\n",
				 __func__, marcel_nbprocessors, ma__nb_vp);

	if (ma_topo_type_depth[MARCEL_LEVEL_NODE] != -1) {
		/* Assume this level is the node level.  */
		unsigned node_level = ma_topo_type_depth[MARCEL_LEVEL_NODE];

		marcel_nbnodes = marcel_topo_level_nbitems[node_level];
		/* Link the marcel_topo_node_level pointer to the `node level' in
			 the marcel_topo_levels tree. */
		marcel_topo_node_level = marcel_topo_levels[node_level];
	}	else {
		marcel_nbnodes = 1;
	}

	/* Update `marcel_vps_per_cpu' et al. accordingly.  */
	distribute_vps();

	/* Initialize information associated with VPs.  */
	for (vp = &marcel_topo_vp_level[0];
			 vp < &marcel_topo_vp_level[ma__nb_vp + MARCEL_NBMAXVPSUP];
			 vp++)
		vp->vpdata =
			(struct marcel_topo_vpdata) MARCEL_TOPO_VPDATA_INITIALIZER(&vp->vpdata);

	mdebug("synthetic topology: %u levels, %u VPs, %u processors\n",
				 marcel_topo_nblevels, ma__nb_vp, marcel_nbprocessors);

	return root;
}

static void
synth_install_topology (void) {
	struct marcel_topo_level *topology;

	topology = synth_make_simple_topology(ma_synthetic_topology_description);
}

#endif /* MA__NUMA */


/* Topology initialization.  */

static void
initialize_topology(void) {
	if (!ma_use_synthetic_topology || ma_synthetic_topology_description[0] == 0)
		topo_discover();
#ifdef MA__NUMA
	else
		synth_install_topology();
#endif

	/* Now we can set VP levels for main LWP */
	ma_init_lwp_vpnum(0, &__main_lwp);
}

__ma_initfunc(initialize_topology, MA_INIT_TOPOLOGY, "Finding Topology");


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
	if (marcel_topo_core_level) {
		for (i=0; marcel_topo_core_level[i].cpuset; i++) {
			if (marcel_vpset_isset(&marcel_topo_core_level[i].cpuset,ma_vpnum(lwp))) {
				ma_per_lwp(core_level,lwp) = &marcel_topo_core_level[i];
				break;
			}
		}
	}
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

static void topology_lwp_start(ma_lwp_t lwp TBX_UNUSED) {
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(topology, 400, "Topology",
				  topology_lwp_init, "Initialisation de la topologie",
				  topology_lwp_start, "Activation de la topologie");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(topology, MA_INIT_TOPOLOGY);

#endif /* MA__LWPS */
