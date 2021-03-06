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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>
#include "sys/marcel_hwlocapi.h"


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

#include "marcel.h"


unsigned marcel_topo_nblevels =
#ifndef MA__LWPS
	0
#else
	1
#endif
	;

struct marcel_topo_level marcel_machine_level[1 + MARCEL_NBMAXVPSUP + 1] = {
	{
		.type = MARCEL_LEVEL_MACHINE,
		.merged_type = 1 << MARCEL_LEVEL_MACHINE,
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
#ifdef MARCEL_SMT_ENABLED
		.nbidle = MA_ATOMIC_INIT(0),
#endif
#ifdef MA__LWPS
		.kmutex = MARCEL_KTHREAD_MUTEX_INITIALIZER,
		.kneed = MARCEL_KTHREAD_COND_INITIALIZER,
		.kneeddone = MARCEL_KTHREAD_COND_INITIALIZER,

		.blocked = 0,
		.spare = 0,
		.needed = -1,
#endif
		.vpdata = MARCEL_TOPO_VPDATA_INITIALIZER(&marcel_machine_level[0].vpdata),
#ifdef MA__NUMA
		.nodedata = MARCEL_TOPO_NODEDATA_INITIALIZER(&marcel_machine_level[0].nodedata),
#endif
	},
	{
		.vpset = MARCEL_VPSET_ZERO,
		.cpuset = MARCEL_VPSET_ZERO,
	}
};

#ifdef MA__NUMA
static struct marcel_topo_level *marcel_topo_cpu_level;

struct marcel_topo_level *ma_vp_die_level[MA_NR_VPS];
struct marcel_topo_level *ma_vp_node_level[MA_NR_VPS];
struct marcel_topo_level *ma_vp_core_level[MA_NR_VPS];

struct marcel_topo_level *marcel_topo_core_level = marcel_machine_level;
struct marcel_topo_level *marcel_topo_node_level = marcel_machine_level;
#endif

#ifdef MA__LWPS
struct marcel_topo_level *marcel_topo_vp_level = marcel_machine_level;
#endif

unsigned marcel_nbnodes = 1;
unsigned marcel_topo_level_nbitems[2 * MARCEL_LEVEL_LAST + 1] = { 1 };

struct marcel_topo_level *marcel_topo_levels[2 * MARCEL_LEVEL_LAST + 1] = {
	marcel_machine_level
};


/* Getting the number of processors.  */

#ifdef MA__LWPS
/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `ma_topology_set_fsys_root ()' to
   have the desired effect. FIXME for hwloc? */
static unsigned ma_fallback_nbprocessors(void)
{
#if defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
	return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF)
	return sysconf(_SC_NPROC_CONF);
#else
#warning No known way to discover number of available processors on this system
#warning ma_fallback_nbprocessors will default to 1
#warning use the --marcel-nvp option to set it by hand when running your program
	return 1;
#endif
}

#endif


/* Support for fake topologies.  */

#ifdef MA__NUMA
tbx_bool_t marcel_use_fake_topology = tbx_true;
#endif



#ifdef MARCEL_LIBPTHREAD
int marcel_cpuset2vpset(size_t cpusetsize, const cpu_set_t * cpuset,
			marcel_vpset_t * vpset)
{
#ifndef DARWIN_SYS
	unsigned int i;
	marcel_vpset_zero(vpset);
	for (i = 0; i < cpusetsize * CHAR_BIT; i++) {
		if (CPU_ISSET(i, cpuset)) {
			if (i >= MARCEL_NBMAXCPUS) {
#ifdef MA__DEBUG
				MARCEL_LOG("cpuset2vpset: invalid VP %d\n", i);
#endif
				return EINVAL;
			}
			marcel_vpset_set(vpset, i);
		} else {
			if (i < MARCEL_NBMAXCPUS)
				marcel_vpset_clr(vpset, i);
		}
	}
#else
	vpset = cpuset;
#endif
	return 0;
}

/* TODO: cleaner fix */
#ifndef CPU_ZERO_S
#  define CPU_ZERO_S(cpusetsize, cpuset) CPU_ZERO(cpuset)
#endif
int marcel_vpset2cpuset(const marcel_vpset_t * vpset, size_t cpusetsize, cpu_set_t * cpuset)
{
#ifndef DARWIN_SYS
	unsigned int i;
	CPU_ZERO_S(cpusetsize, cpuset);
	for (i = 0; i < MARCEL_NBMAXCPUS; i++) {
		if (marcel_vpset_isset(vpset, i)) {
			if (i >= cpusetsize * CHAR_BIT) {
#ifdef MA__DEBUG
				MARCEL_LOG("cpuset2vpset: VP %d beyond user-provided buffer\n", i);
#endif
				return EINVAL;
			}
			CPU_SET(i, cpuset);
		} else {
			if (i < cpusetsize * CHAR_BIT)
				CPU_CLR(i, cpuset);
		}
	}
#else
	cpuset = vpset;
#endif
	return 0;
}
#endif /** MARCEL_LIBPTHREAD **/


int marcel_current_vp(void)
{
	return ma_vpnum(MA_LWP_SELF);
}

int marcel_current_os_node(void)
{
	return ma_current_os_node();
}

marcel_topo_level_t *marcel_topo_level(unsigned level, unsigned index)
{
	if (index > marcel_topo_level_nbitems[level])
		return NULL;
	return &marcel_topo_levels[level][index];
}

const char *marcel_topo_level_string(enum marcel_topo_level_e l)
{
	switch (l) {
	case MARCEL_LEVEL_MACHINE:
		return "Machine";
#ifdef MA__LWPS
#  ifdef MA__NUMA
	case MARCEL_LEVEL_FAKE:
		return "Fake";
	case MARCEL_LEVEL_MISC:
		return "Misc";
	case MARCEL_LEVEL_NODE:
		return "NUMANode";
	case MARCEL_LEVEL_DIE:
		return "Die";
	case MARCEL_LEVEL_L3:
		return "L3Cache";
	case MARCEL_LEVEL_L2:
		return "L2Cache";
	case MARCEL_LEVEL_CORE:
		return "Core";
	case MARCEL_LEVEL_L1:
		return "L1Cache";
	case MARCEL_LEVEL_PROC:
		return "SMTproc";
#  endif
	case MARCEL_LEVEL_VP:
		return "VP";
#endif
	}
	return "Unknown";
}

static void ma_print_level_description(struct marcel_topo_level *l, FILE * output,
				       int verbose_mode)
{
	unsigned long type = l->merged_type;
	const char *separator = " + ";
	const char *current_separator = "";	/* not prefix for the first one */

	if (!verbose_mode) {
#ifdef MA__LWPS
		/* don't print "vp" if there's something else (including "fake") */
		if (type & ~(1 << MARCEL_LEVEL_VP))
			type &= ~(1 << MARCEL_LEVEL_VP);
#  ifdef MA__NUMA
		/* don't print "fake" if there's something else */
		if (type & ~(1 << MARCEL_LEVEL_FAKE))
			type &= ~(1 << MARCEL_LEVEL_FAKE);
		/* don't print "misc" if there's something else */
		if (type & ~(1 << MARCEL_LEVEL_MISC))
			type &= ~(1 << MARCEL_LEVEL_MISC);
		/* don't print smtproc or caches if there's also core or die */
		if (type & ((1 << MARCEL_LEVEL_CORE) | (1 << MARCEL_LEVEL_DIE)))
			type &=
				~((1 << MARCEL_LEVEL_PROC) | (1 << MARCEL_LEVEL_L1) |
				  (1 << MARCEL_LEVEL_L2) | (1 << MARCEL_LEVEL_L3));
#  endif
#endif
	}
#define marcel_print_level_description_level(_l)			\
	if (type & (1<<_l)) {						\
		marcel_fprintf(output, "%s%s", current_separator, marcel_topo_level_string(_l)); \
		current_separator = separator;				\
	}

	marcel_print_level_description_level(MARCEL_LEVEL_MACHINE)
#ifdef MA__LWPS
#  ifdef MA__NUMA
		marcel_print_level_description_level(MARCEL_LEVEL_FAKE)
		marcel_print_level_description_level(MARCEL_LEVEL_MISC)
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

#define marcel_memory_size_printf_value(_size)				\
	(_size)%1024 ? (_size) : (_size)%(1024*1024) ? (_size)>>10 : (_size)>>20
#define marcel_memory_size_printf_unit(_size)			\
	(_size)%1024 ? "kB" : (_size)%(1024*1024) ? "MB" : "GB"
#define MA_PHYSINDEX_STRLEN 12	/* prefix character + 10 digits + ending zero */
#define marcel_physindex_printf(_prefix, _index)			\
	({ char *__tmp = alloca(MA_PHYSINDEX_STRLEN); __tmp[0] = '\0';	\
		if (_index != -1) marcel_snprintf(__tmp, MA_PHYSINDEX_STRLEN, "%s%d", _prefix, _index); __tmp; })

void marcel_print_level(struct marcel_topo_level *l, FILE * output, int verbose_mode,
			const char *separator, const char *indexprefix,
			const char *labelseparator, const char *levelterm)
{
	ma_print_level_description(l, output, verbose_mode);
	marcel_fprintf(output, "%s", labelseparator);
#ifdef MA__NUMA
	if (l->merged_type & (1 << MARCEL_LEVEL_NODE))
		marcel_fprintf(output, "%sNode%s(%lu%s)", separator,
			       marcel_physindex_printf(indexprefix, l->os_node),
			       marcel_memory_size_printf_value(l->memory_kB
							       [MARCEL_TOPO_LEVEL_MEMORY_NODE]),
			       marcel_memory_size_printf_unit(l->memory_kB
							      [MARCEL_TOPO_LEVEL_MEMORY_NODE]));
	if (l->merged_type & (1 << MARCEL_LEVEL_DIE))
		marcel_fprintf(output, "%sDie%s", separator,
			       marcel_physindex_printf(indexprefix, l->os_die));
	if (l->merged_type & (1 << MARCEL_LEVEL_L3))
		marcel_fprintf(output, "%sL3%s(%lu%s)", separator,
			       marcel_physindex_printf(indexprefix, l->os_l3),
			       marcel_memory_size_printf_value(l->memory_kB
							       [MARCEL_TOPO_LEVEL_MEMORY_L3]),
			       marcel_memory_size_printf_unit(l->memory_kB
							      [MARCEL_TOPO_LEVEL_MEMORY_L3]));
	if (l->merged_type & (1 << MARCEL_LEVEL_L2))
		marcel_fprintf(output, "%sL2%s(%lu%s)", separator,
			       marcel_physindex_printf(indexprefix, l->os_l2),
			       marcel_memory_size_printf_value(l->memory_kB
							       [MARCEL_TOPO_LEVEL_MEMORY_L2]),
			       marcel_memory_size_printf_unit(l->memory_kB
							      [MARCEL_TOPO_LEVEL_MEMORY_L2]));
	if (l->merged_type & (1 << MARCEL_LEVEL_CORE))
		marcel_fprintf(output, "%sCore%s", separator,
			       marcel_physindex_printf(indexprefix, l->os_core));
	if (l->merged_type & (1 << MARCEL_LEVEL_L1))
		marcel_fprintf(output, "%sL1%s(%lu%s)", separator,
			       marcel_physindex_printf(indexprefix, l->os_l1),
			       marcel_memory_size_printf_value(l->memory_kB
							       [MARCEL_TOPO_LEVEL_MEMORY_L1]),
			       marcel_memory_size_printf_unit(l->memory_kB
							      [MARCEL_TOPO_LEVEL_MEMORY_L1]));
	if (l->merged_type & (1 << MARCEL_LEVEL_PROC))
		marcel_fprintf(output, "%sCPU%s", separator,
			       marcel_physindex_printf(indexprefix, l->os_cpu));
#endif
	if (l->level == marcel_topo_nblevels - 1) {
		marcel_fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
	}
	marcel_fprintf(output, "%s\n", levelterm);
}


#ifdef MA__LWPS
unsigned marcel_nbprocessors = 1;
unsigned marcel_cpu_stride = 0;
unsigned marcel_first_cpu = 0;
unsigned marcel_vps_per_cpu = 1;
#  ifdef MA__NUMA
unsigned marcel_topo_max_arity = 4;
static unsigned marcel_topo_merge = 1;
#  endif


/** \brief Compute ::marcel_vps_per_cpu, and ::marcel_cpu_stride if not
 *  already set */
static void ma_distribute_vps(void)
{
	if (0 == marcel_nb_vp)
		marcel_nb_vp = marcel_nbprocessors / marcel_cpu_stride;

	marcel_vps_per_cpu = (marcel_nb_vp + marcel_nbprocessors - 1) / marcel_nbprocessors;
	if (marcel_cpu_stride > 1 && marcel_vps_per_cpu > 1) {
		MA_WARN_USER("More VPs that CPUs doesn't make sense with a stride\n");
		exit(1);
	}

	MARCEL_LOG("%d LWP%s per cpu, stride %d, from %d\n", marcel_vps_per_cpu,
		   marcel_vps_per_cpu == 1 ? "" : "s", marcel_cpu_stride,
		   marcel_first_cpu);
}

#  ifdef MA__NUMA

static int ma_topo_type_depth[MARCEL_LEVEL_LAST + 1];

int ma_get_topo_type_depth(enum marcel_topo_level_e type)
{
	return ma_topo_type_depth[type];
}

marcel_topo_level_t *marcel_topo_common_ancestor(marcel_topo_level_t * lvl1,
						 marcel_topo_level_t * lvl2)
{
	marcel_topo_level_t *l1 = NULL, *l2 = NULL;

	MA_BUG_ON(!lvl1 || !lvl2);

	/* There can't be any upper level in that case. */
	if (!lvl1->father)
		return lvl1;
	if (!lvl2->father)
		return lvl2;

	/* We always store the lower-depthed topo_level (the upper level in
	   the topo_level hierarchy) in l1 to simplify the following
	   code. */
	l1 = (lvl1->level < lvl2->level) ? lvl1 : lvl2;
	l2 = (lvl1->level < lvl2->level) ? lvl2 : lvl1;

	/* We already know that l1 is the upper topo_level, let's
	   lift l2 up to the same level before going any further. */
	while (l1->level < l2->level)
		l2 = l2->father;

	/* l1 and l2 should be at the same level now. */
	MA_BUG_ON(l1->level != l2->level);

	/* Maybe l2 is the one we're looking for! (i.e. l1 was originally
	   somewhere behind l2, l2 is so the lower ancestor) */
	if (l2 == l1)
		return l2;

	/* If it's not the case, we have to look up until we find fathers
	   with the same index (same level + same index = same
	   topo_level! */
	while (l2 != l1) {
		MA_BUG_ON(!l1->father || !l2->father);
		l1 = l1->father;
		l2 = l2->father;
	}
	return l2;
}
#  endif /* MA__NUMA */

/* Returns true if _level_ is inside the subtree beginning with
   _subtree_root_. */
#  ifdef MA__NUMA
int ma_topo_is_in_subtree(marcel_topo_level_t * subtree_root, marcel_topo_level_t * level)
{
	return marcel_topo_common_ancestor(subtree_root, level) == subtree_root;
}
#  endif /** MA_NUMA **/

#  ifdef MA__NUMA
/* FIXME: move this knowledge inside libtopo? */
static unsigned *ma_split_quirk(const char *dmi_board_vendor, const char *dmi_board_name,
				enum marcel_topo_level_e ptype, unsigned arity,
				enum marcel_topo_level_e ctype, unsigned *subarity,
				unsigned *sublevels)
{
	unsigned *array = NULL;
	if (ptype == MARCEL_LEVEL_MACHINE && ctype == MARCEL_LEVEL_NODE) {
		/* splitting machine->numanode arity */

		/* quirk for tyan 8-opteron machines */
		if (arity == 8
		    && dmi_board_vendor && dmi_board_name
		    && !strncasecmp(dmi_board_vendor, "tyan", 4)
		    && (!strncasecmp(dmi_board_name, "S4881", 5)
			|| !strncasecmp(dmi_board_name, "S4885", 5)
			|| !strncasecmp(dmi_board_name, "S4985", 5))) {

			MARCEL_LOG
				("splitting machine->numanode on Tyan 8 opteron board, apply split quirk\n");

			array = marcel_malloc(8 * sizeof(*array));
			MA_ALWAYS_BUG_ON(!array);

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
	}

	return array;
}

/* split arity into nbsublevels of size sublevelarity */
static void split(unsigned arity, unsigned *__restrict sublevelarity,
		  unsigned *__restrict nbsublevels)
{
	if (!arity) {
		*sublevelarity = *nbsublevels = 0;
		return;
	}

	*sublevelarity = (unsigned)sqrt(arity);
	*nbsublevels = (arity + *sublevelarity - 1) / *sublevelarity;
	if (*nbsublevels > marcel_topo_max_arity) {
		*nbsublevels = marcel_topo_max_arity;
		*sublevelarity = (arity + *nbsublevels - 1) / *nbsublevels;
	}
}
#  endif /** MA__NUMA **/

/* Connect levels */
static void topo_connect(void)
{
	int i, j;
	unsigned l, m;
	for (l = 0; l < marcel_topo_nblevels - 1; l++) {
		for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset); i++) {
			if (marcel_topo_levels[l][i].arity) {
				MARCEL_LOG("level %u,%u: cpuset %" MARCEL_PRIxVPSET
					   " arity %u\n", l, i,
					   MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels[l][i].cpuset),
					   marcel_topo_levels[l][i].arity);
				marcel_topo_levels[l][i].children =
					marcel_malloc(marcel_topo_levels[l][i].arity * sizeof(void *));
				MA_ALWAYS_BUG_ON(!marcel_topo_levels[l][i].children);

				m = 0;
				for (j = 0; !marcel_vpset_iszero(&marcel_topo_levels[l + 1][j].cpuset); j++)
					if (marcel_vpset_isincluded(&marcel_topo_levels[l][i].cpuset,
								    &marcel_topo_levels[l + 1][j].cpuset)) {
						MA_ALWAYS_BUG_ON(m >= marcel_topo_levels[l][i].arity);
						marcel_topo_levels[l][i].children[m] =
							&marcel_topo_levels[l + 1][j];
						marcel_topo_levels[l + 1][j].father =
							&marcel_topo_levels[l][i];
						marcel_topo_levels[l + 1][j].index = m++;
					}
			}
		}
	}
}

#ifdef MA__NUMA
static enum marcel_topo_level_e ma_topo_level_type_from_hwloc(hwloc_obj_t t)
{
	enum marcel_topo_level_e mtype = MARCEL_LEVEL_MACHINE;	/* default value which always exists */

	switch (t->type) {
		/* only support topologies with a single machine */
	case HWLOC_OBJ_MACHINE:
		mtype = MARCEL_LEVEL_MACHINE;
		break;
	case HWLOC_OBJ_SYSTEM:
		MA_WARN_USER("SSI OSes like Kerrighed not supported yet\n");
		MA_ALWAYS_BUG_ON(1);
	case HWLOC_OBJ_NODE:
		mtype = MARCEL_LEVEL_NODE;
		break;
	case HWLOC_OBJ_SOCKET:
		mtype = MARCEL_LEVEL_DIE;
		break;
	case HWLOC_OBJ_CORE:
		mtype = MARCEL_LEVEL_CORE;
		break;
	case HWLOC_OBJ_PU:
		mtype = MARCEL_LEVEL_PROC;
		break;
	case HWLOC_OBJ_MISC:
		mtype = MARCEL_LEVEL_MISC;
		break;
	case HWLOC_OBJ_GROUP:
		mtype = MARCEL_LEVEL_MISC;
		break;
	case HWLOC_OBJ_CACHE:{
		switch (t->attr->cache.depth) {
		case 1:
			mtype = MARCEL_LEVEL_L1;
			break;
		case 2:
			mtype = MARCEL_LEVEL_L2;
			break;
		case 3:
			mtype = MARCEL_LEVEL_L3;
			break;
		default:
			/* FIXME ignore other cache depth */
			MA_WARN_USER("Cannot convert hwloc cache depth %u\n",
				     t->attr->cache.depth);
			MA_ALWAYS_BUG_ON(1);
		}
		break;
	}

	default:
		/* MAX and other should not occur */
		MA_WARN_USER("Cannot convert hwloc type %u\n", t->type);
		MA_ALWAYS_BUG_ON(1);
	}

	return mtype;
}
#  endif				/* MA__NUMA */

/* Main discovery loop */
static void topo_discover(void)
{
	unsigned i, j, l;
#  ifdef MA__NUMA
	unsigned m, n;
	int dosplit;
	unsigned sublevelarity, nbsublevels;
	char *dmi_board_vendor = NULL, *dmi_board_name = NULL;
	unsigned int nb_physical_processors = 0, nb_logical_processors = 0;
#  endif
	struct marcel_topo_level *level;

	/* Initialize the number of processor to some reasonable default, e.g.,
	   obtained using sysconf(3).  */
	marcel_nbprocessors = ma_fallback_nbprocessors();

	if (marcel_nbvps() > MARCEL_NBMAXCPUS) {
		MA_WARN_USER("nbvps(%u) > NBMAXCPUS(%d), please increase the number of CPUs supported: reconfigure Marcel with option --with-nbmaxcpus=value", marcel_nbvps(), MARCEL_NBMAXCPUS);
		exit(1);
	}

	/* Raw detection, from coarser levels to finer levels */
#  ifdef MA__NUMA
	unsigned topodepth = hwloc_topology_get_depth(topology);

	marcel_use_fake_topology =
		hwloc_topology_is_thissystem(topology) ? tbx_false : tbx_true;
	if (synthetic_topology_description) {
		marcel_topo_max_arity = -1;	/* Synthetic topologies do not need splitting */
		marcel_topo_merge = 0;	/* Synthetic topologies do not need merging */
	}
	/* starting with hwloc 1.0, all topologies support by marcel 
	 * have a MACHINE object on top */
	MA_ALWAYS_BUG_ON(hwloc_get_root_obj(topology)->type != HWLOC_OBJ_MACHINE);

	for (l = topodepth - 1; l != (unsigned) -1 && l < topodepth; l--) {
		struct marcel_topo_level *mlevels;
		unsigned int nbitems = hwloc_get_nbobjs_by_depth(topology, l);
		hwloc_obj_type_t ltype = hwloc_get_depth_type(topology, l);

		MARCEL_LOG("converting %d items from hwloc level depth %d (type %d)\n",
			   nbitems, l, ltype);

		if (ltype == HWLOC_OBJ_MACHINE) {
			mlevels = marcel_machine_level;
		} else {
			mlevels = marcel_malloc((nbitems + MARCEL_NBMAXVPSUP + 1) * sizeof(*mlevels));
			MA_ALWAYS_BUG_ON(!mlevels);
			marcel_topo_level_nbitems[l] = nbitems;
			marcel_topo_levels[l] = mlevels;
			switch (ltype) {
			case HWLOC_OBJ_NODE:
				marcel_topo_node_level = mlevels;
				marcel_nbnodes = nbitems;
				break;
			case HWLOC_OBJ_CORE:
				marcel_topo_core_level = mlevels;
				break;
			case HWLOC_OBJ_PU:
				marcel_topo_cpu_level = mlevels;
				break;
			default:
				break;
			}
		}

		/* FIXME: drop level if no numa? */

		for (i = 0; i < nbitems; i++) {
			hwloc_obj_t tlevel = hwloc_get_obj_by_depth(topology, l, i);
			struct marcel_topo_level *mlevel = &mlevels[i];
			enum marcel_topo_level_e mtype =
				ma_topo_level_type_from_hwloc(tlevel);

			ma_topo_setup_level(mlevel, mtype);
			MA_ALWAYS_BUG_ON(tlevel->depth != l);
			MA_ALWAYS_BUG_ON(tlevel->logical_index != i);
			mlevel->index =	hwloc_get_obj_by_depth(topology, l, i)->sibling_rank;

			ma_topo_set_empty_os_numbers(mlevel);
			switch (mtype) {
			case MARCEL_LEVEL_MACHINE:
				mlevel->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_MACHINE] =
					tlevel->memory.total_memory >> 10;
				if (tlevel->memory.page_types_len >= 2) {
					mlevel->huge_page_free =
						tlevel->memory.page_types[1].count;
					mlevel->huge_page_size =
						tlevel->memory.page_types[1].size;
				} else {
					mlevel->huge_page_free = 0;
					mlevel->huge_page_size = 0;
				}
				ma_hwlocapi_get_board_info(tlevel, dmi_board_name, dmi_board_vendor)
				break;
			case MARCEL_LEVEL_NODE:
				mlevel->os_node = tlevel->os_index;
				mlevel->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE] =
					tlevel->memory.total_memory >> 10;
				if (tlevel->memory.page_types_len >= 2) {
					mlevel->huge_page_free =
						tlevel->memory.page_types[1].count;
					mlevel->huge_page_size =
						tlevel->memory.page_types[1].size;
				} else {
					mlevel->huge_page_free = 0;
					mlevel->huge_page_size = 0;
				}
				break;
			case MARCEL_LEVEL_DIE:
				mlevel->os_die = tlevel->os_index;
				break;
			case MARCEL_LEVEL_L3:
				mlevel->os_l3 = tlevel->os_index;
				mlevel->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L3] =
					tlevel->attr->cache.size >> 10;
				break;
			case MARCEL_LEVEL_L2:
				mlevel->os_l2 = tlevel->os_index;
				mlevel->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L2] =
					tlevel->attr->cache.size >> 10;
				break;
			case MARCEL_LEVEL_CORE:
				/** number of physical CPUs available **/
				nb_physical_processors = hwloc_get_nbobjs_by_depth(topology, l);

				mlevel->os_core = tlevel->os_index;
#ifdef MARCEL_SMT_ENABLED
				ma_atomic_set(&mlevel->nbidle, 0);
#endif
				break;
			case MARCEL_LEVEL_L1:
				mlevel->os_l1 = tlevel->os_index;
				mlevel->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L1] =
					tlevel->attr->cache.size >> 10;
				break;
			case MARCEL_LEVEL_PROC:
				/** number of logical CPUs available **/
				nb_logical_processors = hwloc_get_nbobjs_by_depth(topology, l);

				mlevel->os_cpu = tlevel->os_index;
				if (mlevel->os_cpu >= MARCEL_NBMAXCPUS) {
					/* these won't fit in marcel's vpset type and thus would abort strangely later */
					if (marcel_nbvps() > MARCEL_NBMAXCPUS) {
						MA_WARN_USER("CPU id %d >= NBMAXCPUS(%d), please increase the number of CPUs supported: reconfigure Marcel with option --with-nbmaxcpus=value", mlevel->os_cpu, MARCEL_NBMAXCPUS);
						exit(1);
					}
				}
				break;
			default:
				break;
			}

			ma_cpuset_from_hwloc(&mlevel->cpuset, tlevel->cpuset);

			marcel_vpset_zero(&mlevel->vpset);

			MARCEL_LOG("assembled hwloc level at depth %d cpuset %"
				   MARCEL_PRIxVPSET "\n", l,
				   MARCEL_VPSET_PRINTF_VALUE(mlevel->cpuset));
		}
		marcel_vpset_zero(&mlevels[i].vpset);
		marcel_vpset_zero(&mlevels[i].cpuset);
	}

	/** Some OSs hide/don't know physical processors number (ie show only logical CPUs) */
	if (0 == nb_physical_processors)
		nb_physical_processors = nb_logical_processors;

	/** Set marcel_nbprocessors **/
	marcel_nbprocessors = nb_logical_processors;
	MA_ALWAYS_BUG_ON(!marcel_nbprocessors);

#  ifndef MARCEL_SMT_ENABLED
	/** Set marcel_cpu_stride if user does not set it 
	    and if user does not set vp number **/
	if (0 == marcel_cpu_stride && 0 == marcel_nb_vp)
		marcel_cpu_stride = nb_logical_processors / nb_physical_processors;
#  endif

	/* FIXME: drop level if not ordered as expected */
	marcel_topo_nblevels = topodepth;
	MARCEL_LOG("\n\n--> discovered %d levels\n\n", marcel_topo_nblevels);
#endif

	/** Set marcel_cpu_stride if user does not set it **/
	if (0 == marcel_cpu_stride)
		marcel_cpu_stride = 1;
	MA_ALWAYS_BUG_ON(!marcel_cpu_stride);

	/* Set the number of VPs */
	ma_distribute_vps();

	MARCEL_LOG("%s: chose %u VPs\n", __func__, marcel_nb_vp);
	if (marcel_nbvps() > MARCEL_NBMAXCPUS) {
		MA_WARN_USER("nbvps(%u) > NBMAXCPUS(%d), please increase the number of CPUs supported: reconfigure Marcel with option --with-nbmaxcpus=value", marcel_nbvps(), MARCEL_NBMAXCPUS);
		exit(1);
	}

	/* TODO: Brice will probably want the OS->VP function */

	/* create the VP level: now we decide which CPUs we will really use according to nvp and stride */
	struct marcel_topo_level *vp_level =
		marcel_malloc((marcel_nbvps() + MARCEL_NBMAXVPSUP + 1) * sizeof(*vp_level));
	MA_ALWAYS_BUG_ON(!vp_level);

	marcel_vpset_t cpuset = MARCEL_VPSET_ZERO;
	/* do not initialize supplementary VPs yet, since they may end up on the machine level on a single-CPU machine */
	unsigned cpu = marcel_first_cpu;
	unsigned vps = 0;

	MARCEL_LOG("\n\n * VP mapping details *\n\n");
	for (i = 0; i < marcel_nbvps(); i++) {
#  ifdef MA__NUMA
		MA_ALWAYS_BUG_ON(cpu >= marcel_nbprocessors);
		marcel_vpset_t oscpuset =
			marcel_topo_levels[marcel_topo_nblevels - 1][cpu].cpuset;
		MA_ALWAYS_BUG_ON(marcel_vpset_weight(&oscpuset) != 1);
		unsigned oscpu = marcel_vpset_first(&oscpuset);
#  else
		unsigned oscpu = cpu;
#  endif
		ma_topo_setup_level(&vp_level[i], MARCEL_LEVEL_VP);
		ma_topo_set_os_numbers(&vp_level[i], cpu, oscpu);

		marcel_vpset_vp(&vp_level[i].cpuset, oscpu);
		marcel_vpset_set(&cpuset, oscpu);

		MARCEL_LOG("VP %d on %dth proc with cpuset %" MARCEL_PRIxVPSET "\n",
			   i, cpu, MARCEL_VPSET_PRINTF_VALUE(vp_level[i].cpuset));

		/* Follow the machine as nicely as possible, for instance, with two bicore chips and 6 vps:
		 * chips: [     ] [     ] *
		 * cpus:  [ ] [ ] [ ] [ ] *
		 * VPs:    0   2   4   5  *
		 *         1   3          */
		int stopfill =
			i >=
			(((marcel_nbvps() - 1) % marcel_nbprocessors +
			  1) * marcel_vps_per_cpu);
		if (++vps == marcel_vps_per_cpu - stopfill) {
			cpu += marcel_cpu_stride;
			vps = 0;
		}
	}
	marcel_vpset_zero(&vp_level[i].cpuset);
	marcel_vpset_zero(&vp_level[i].vpset);

	/* And add this one */
	marcel_topo_level_nbitems[marcel_topo_nblevels] = marcel_nbvps();
	marcel_topo_levels[marcel_topo_nblevels++] = vp_level;

	/* Now filter out CPUs from levels. */
	for (l = 0; l < marcel_topo_nblevels; l++) {
		for (i = 0; i < marcel_topo_level_nbitems[l]; i++) {
			marcel_vpset_andset(&marcel_topo_levels[l][i].cpuset, &cpuset);
			MARCEL_LOG("level %u,%u: cpuset becomes %" MARCEL_PRIxVPSET "\n",
				   l, i, MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels[l]
								   [i].cpuset));
			if (marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset)) {
				MARCEL_LOG("became empty, dropping it\n");
				marcel_topo_level_nbitems[l]--;
				memmove(&marcel_topo_levels[l][i],
					&marcel_topo_levels[l][i + 1],
					sizeof(marcel_topo_levels[l][i]) *
					(marcel_topo_level_nbitems[l] - i + 1));
				i--;
			}
		}
	}

	/* Now we can put numbers on levels. */
	for (l = 0; l < marcel_topo_nblevels; l++)
		for (i = 0; i < marcel_topo_level_nbitems[l]; i++) {
			marcel_topo_levels[l][i].number = i;
			MARCEL_LOG("level %u,%u: cpuset %" MARCEL_PRIxVPSET "\n",
				   l, i, MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels[l]
								   [i].cpuset));
		}

	/* And show debug again */
	for (l = 0; l < marcel_topo_nblevels; l++)
		for (i = 0; i < marcel_topo_level_nbitems[l]; i++)
			MARCEL_LOG("level %u,%u: cpuset %" MARCEL_PRIxVPSET "\n",
				   l, i, MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels[l]
								   [i].cpuset));

#  ifdef MA__NUMA
	/* merge identical levels */
	if (marcel_topo_merge) {
		for (l = 0; l + 1 < marcel_topo_nblevels; l++) {
			for (i = 0;
			     !marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset); i++);
			for (j = 0;
			     j < i
				     &&
				     !marcel_vpset_iszero(&marcel_topo_levels[l + 1][j].cpuset);
			     j++)
				if (!marcel_vpset_isequal
				    (&marcel_topo_levels[l + 1][j].cpuset,
				     &marcel_topo_levels[l][j].cpuset))
					break;
			if (j == i
			    && marcel_vpset_iszero(&marcel_topo_levels[l + 1][j].cpuset))
			{
				MARCEL_LOG
					("merging levels %u and %u since same %d item%s\n", l,
					 l + 1, i, i >= 2 ? "s" : "");
				if (marcel_topo_levels[l + 1] == marcel_topo_cpu_level)
					marcel_topo_cpu_level = marcel_topo_levels[l];
				else if (marcel_topo_levels[l + 1] ==
					 marcel_topo_core_level)
					marcel_topo_core_level = marcel_topo_levels[l];
				else if (marcel_topo_levels[l + 1] ==
					 marcel_topo_node_level)
					marcel_topo_node_level = marcel_topo_levels[l];
				for (i = 0; i < marcel_topo_level_nbitems[l]; i++) {
#    define merge_os_components(component)				\
					if (marcel_topo_levels[l][i].os_##component == -1) \
						marcel_topo_levels[l][i].os_##component = marcel_topo_levels[l+1][i].os_##component; \
					else				\
						if (marcel_topo_levels[l+1][i].os_##component != -1) \
							MA_ALWAYS_BUG_ON(marcel_topo_levels[l][i].os_##component != marcel_topo_levels[l+1][i].os_##component);
					merge_os_components(node);
					merge_os_components(die);
					merge_os_components(l3);
					merge_os_components(l2);
					merge_os_components(core);
					merge_os_components(l1);
					merge_os_components(cpu);

#    define merge_memory_kB(_type)					\
					if (marcel_topo_levels[l+1][i].merged_type & (1<<MARCEL_LEVEL_##_type)) { \
						MA_ALWAYS_BUG_ON(marcel_topo_levels[l][i].merged_type & (1<<MARCEL_LEVEL_##_type)); \
						marcel_topo_levels[l][i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_##_type] \
							= marcel_topo_levels[l+1][i].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_##_type]; \
					}
					merge_memory_kB(L1);
					merge_memory_kB(L2);
					merge_memory_kB(L3);
					merge_memory_kB(NODE);

					/* merge types at the end so that the above assertions work */
					marcel_topo_levels[l][i].merged_type |=
						marcel_topo_levels[l + 1][i].merged_type;
				}
				marcel_free(marcel_topo_levels[l + 1]);
				memmove(&marcel_topo_level_nbitems[l + 1],
					&marcel_topo_level_nbitems[l + 2],
					(marcel_topo_nblevels -
					 (l + 2)) * sizeof(*marcel_topo_level_nbitems));
				memmove(&marcel_topo_levels[l + 1],
					&marcel_topo_levels[l + 2],
					(marcel_topo_nblevels -
					 (l + 2)) * sizeof(*marcel_topo_levels));
				marcel_topo_nblevels--;
				marcel_topo_levels[marcel_topo_nblevels] = NULL;
				l--;
			}
		}
	}
#  endif			/* MA__NUMA */

	/* Compute arity */
	for (l = 0; l + 1 < marcel_topo_nblevels; l++) {
		for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset); i++) {
			marcel_topo_levels[l][i].arity = 0;
			for (j = 0;
			     !marcel_vpset_iszero(&marcel_topo_levels[l + 1][j].cpuset);
			     j++)
				if (marcel_vpset_isincluded
				    (&marcel_topo_levels[l][i].cpuset,
				     &marcel_topo_levels[l + 1][j].cpuset))
					marcel_topo_levels[l][i].arity++;
			MARCEL_LOG("level %u,%u: cpuset %" MARCEL_PRIxVPSET " arity %u\n",
				   l, i, MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels[l]
								   [i].cpuset),
				   marcel_topo_levels[l][i].arity);
		}
	}
	for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[marcel_topo_nblevels - 1]
					 [i].cpuset); i++)
		MARCEL_LOG("level %u,%u: cpuset %" MARCEL_PRIxVPSET " leaf\n",
			   marcel_topo_nblevels - 1, i,
			   MARCEL_VPSET_PRINTF_VALUE(marcel_topo_levels
						     [marcel_topo_nblevels -
						      1][i].cpuset));
	MARCEL_LOG("arity done.\n");

	/* and finally connect levels */
	topo_connect();
	MARCEL_LOG("connecting done.\n");

#  ifdef MA__NUMA
	/* Split hi-arity levels */
	if (marcel_topo_max_arity) {
		/* For each level */
		for (l = 0; l + 1 < marcel_topo_nblevels; l++) {
			unsigned level_width = 0;
			dosplit = 0;
			/* Look at each item, check for max_arity */
			for (i = 0;
			     !marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset);
			     i++) {
				split(marcel_topo_levels[l][i].arity, &sublevelarity,
				      &nbsublevels);
				if (marcel_topo_levels[l][i].arity >
				    marcel_topo_max_arity) {
					MARCEL_LOG
						("will split level %u,%u because %d > %d\n",
						 l, i, marcel_topo_levels[l][i].arity,
						 marcel_topo_max_arity);
					dosplit = 1;
				}
				level_width += nbsublevels;
			}
			if (dosplit) {
				/* split needed, shift levels */
				memmove(&marcel_topo_level_nbitems[l + 2],
					&marcel_topo_level_nbitems[l + 1],
					(marcel_topo_nblevels -
					 (l + 1)) * sizeof(*marcel_topo_level_nbitems));
				memmove(&marcel_topo_levels[l + 2],
					&marcel_topo_levels[l + 1],
					(marcel_topo_nblevels -
					 (l + 1)) * sizeof(*marcel_topo_levels));
				/* new fake level */
				marcel_topo_level_nbitems[l + 1] = level_width;
				marcel_topo_levels[l + 1] =
					marcel_malloc((level_width + MARCEL_NBMAXVPSUP +1) * sizeof(**marcel_topo_levels));
				/* fill it with split items */
				for (i = 0, j = 0;
				     !marcel_vpset_iszero(&marcel_topo_levels[l]
							  [i].cpuset); i++) {
					/* split one item */
					unsigned k;
					unsigned *quirk_array;
					quirk_array =
						ma_split_quirk(dmi_board_vendor,
							       dmi_board_name,
							       marcel_topo_levels[l][i].type,
							       marcel_topo_levels[l][i].arity,
							       marcel_topo_levels[l +
										  2][0].type,
							       &sublevelarity, &nbsublevels);
					if (!quirk_array)
						split(marcel_topo_levels[l][i].arity,
						      &sublevelarity, &nbsublevels);

					MARCEL_LOG
						("splitting level %u,%u into %u sublevels of size at most %u\n",
						 l, i, nbsublevels, sublevelarity);
					/* initialize subitems */
					for (k = 0; k < nbsublevels; k++) {
						level = &marcel_topo_levels[l + 1][j + k];
						level->type = MARCEL_LEVEL_FAKE;
						level->merged_type =
							1 << MARCEL_LEVEL_FAKE;
						level->number = j + k;
						level->index = k;
						ma_topo_set_empty_os_numbers(level);
						marcel_vpset_zero(&level->cpuset);
						marcel_vpset_zero(&level->vpset);
						level->arity = 0;
						level->children =
							marcel_malloc(sublevelarity * sizeof(void *));
						level->father = &marcel_topo_levels[l][i];
					}

					/* distribute cpus to subitems */
					/* give cpus of sublevelarity items to each fake item */

					if (quirk_array) {
						/* use the quirk array to connect new levels */
						for (n = 0;
						     n < marcel_topo_levels[l][i].arity;
						     n++) {
							unsigned quirk_father =
								quirk_array[n];
							unsigned child_index;
							level =
								&marcel_topo_levels[l + 1][j +
											   quirk_father];
							marcel_vpset_orset(&level->cpuset,
									   &marcel_topo_levels
									   [l][i].children
									   [n]->cpuset);
							child_index = level->arity;
							level->children[child_index] =
								marcel_topo_levels[l]
								[i].children[n];
							marcel_topo_levels[l][i].children
								[n]->index = child_index;
							marcel_topo_levels[l][i].children
								[n]->father = level;
							level->arity++;
							MA_ALWAYS_BUG_ON(level->arity >
									 sublevelarity);
						}
						marcel_free(quirk_array);

					} else {
						/* linear split */

						/* first fake item */
						k = 0;
						/* will get first item's cpus */
						m = 0;
						for (n = 0;
						     n < marcel_topo_levels[l][i].arity;
						     n++) {
							level =
								&marcel_topo_levels[l + 1][j +
											   k];
							marcel_vpset_orset(&level->cpuset,
									   &marcel_topo_levels
									   [l][i].children
									   [n]->cpuset);
							level->arity++;
							level->children[m] =
								marcel_topo_levels[l]
								[i].children[n];
							marcel_topo_levels[l][i].children
								[n]->index = m;
							marcel_topo_levels[l][i].children
								[n]->father = level;
							if (++m == sublevelarity) {
								k++;
								m = 0;
							}
						}
						if (m)
							/* Incomplete last level */
							k++;
						MA_ALWAYS_BUG_ON(k != nbsublevels);
					}

					/* reconnect */
					marcel_topo_levels[l][i].arity = nbsublevels;
					marcel_free(marcel_topo_levels[l][i].children);
					marcel_topo_levels[l][i].children =
						marcel_malloc(nbsublevels * sizeof(void *));
					for (k = 0; k < nbsublevels; k++)
						marcel_topo_levels[l][i].children[k] =
							&marcel_topo_levels[l + 1][j + k];

					MARCEL_LOG("now level %u,%u: cpuset %"
						   MARCEL_PRIxVPSET " has arity %u\n", l,
						   i,
						   MARCEL_VPSET_PRINTF_VALUE
						   (marcel_topo_levels[l][i].cpuset),
						   marcel_topo_levels[l][i].arity);
					j += nbsublevels;
				}
				MA_ALWAYS_BUG_ON(j != level_width);
				marcel_vpset_zero(&marcel_topo_levels[l + 1][j].cpuset);
				marcel_vpset_zero(&marcel_topo_levels[l + 1][j].vpset);
				if (++marcel_topo_nblevels ==
				    sizeof(marcel_topo_levels) /
				    sizeof(*marcel_topo_levels))
					MA_BUG();
			}
		}
	}

	/* intialize all depth to unknown */
	for (l = 0; l <= MARCEL_LEVEL_LAST; l++)
		ma_topo_type_depth[l] = -1;

	/* walk the existing levels to set their depth */
	for (l = 0; l < marcel_topo_nblevels; l++)
		ma_topo_type_depth[marcel_topo_levels[l][0].type] = l;

	/* setup the depth of all still unknown levels (the one that got merged or never created */
	int type, prevdepth = -1;
	for (type = 0; type <= MARCEL_LEVEL_LAST; type++) {
		if (ma_topo_type_depth[type] == -1)
			ma_topo_type_depth[type] = prevdepth;
		else
			prevdepth = ma_topo_type_depth[type];
	}
#  endif			/* MA__NUMA */

	MA_ALWAYS_BUG_ON(marcel_topo_level_nbitems[marcel_topo_nblevels - 1] !=
			 marcel_nbvps());
	marcel_topo_vp_level = marcel_topo_levels[marcel_topo_nblevels - 1];

	/* Set VP sets */
	/* This is the only one which isn't empty by default */
	marcel_vpset_zero(&marcel_machine_level[0].vpset);
	for (i = 0; i < marcel_nbvps(); i++) {
		level = &marcel_topo_vp_level[i];
		while (level) {
			marcel_vpset_set(&level->vpset, i);
			level = level->father;
		}
	}

#ifdef MA__NUMA
	/* Fill node access cache */
	if (marcel_topo_node_level)
		for (level = marcel_topo_node_level; !marcel_vpset_iszero(&level->cpuset);
		     level++)
			for (j = 0; j < marcel_nbvps(); j++)
				if (marcel_vpset_isset(&level->vpset, j))
					ma_vp_node_level[j] = level;

	/* Fill die access cache */
	for (level = marcel_topo_cpu_level; level->type > MARCEL_LEVEL_DIE;)
		level = level->father;
	while (!marcel_vpset_iszero(&level->cpuset)) {
		for (j = 0; j < marcel_nbvps(); j++)
			if (marcel_vpset_isset(&level->vpset, j))
				ma_vp_die_level[j] = level;

		level++;
	}
	if (marcel_topo_core_level)
		for (level = marcel_topo_core_level; !marcel_vpset_iszero(&level->cpuset);
		     level++)
			for (j = 0; j < marcel_nbvps(); j++)
				if (marcel_vpset_isset(&level->vpset, j))
					ma_vp_core_level[j] = level;
#endif				/* MA__NUMA */

	/* Now add supplementary VPs on the last level. */
	for (i = marcel_nbvps(); i < marcel_nbvps() + MARCEL_NBMAXVPSUP; i++) {
		marcel_topo_vp_level[i].type = MARCEL_LEVEL_VP;
		marcel_topo_vp_level[i].merged_type = 1 << MARCEL_LEVEL_VP;
		marcel_topo_vp_level[i].number = i;
		ma_topo_set_empty_os_numbers(&marcel_topo_vp_level[i]);
		marcel_vpset_vp(&marcel_topo_vp_level[i].vpset, i);
		marcel_vpset_zero(&marcel_topo_vp_level[i].cpuset);
		marcel_topo_vp_level[i].arity = 0;
		marcel_topo_vp_level[i].children = NULL;
		marcel_topo_vp_level[i].father = NULL;
	}
	marcel_vpset_zero(&marcel_topo_vp_level[i].cpuset);
	marcel_vpset_zero(&marcel_topo_vp_level[i].vpset);

	/* Initialize per-VP data */
	for (level = &marcel_topo_vp_level[0];
	     level < &marcel_topo_vp_level[marcel_nbvps() + MARCEL_NBMAXVPSUP]; level++)
		level->vpdata = (struct marcel_topo_vpdata)
			MARCEL_TOPO_VPDATA_INITIALIZER(&level->vpdata);
#  ifdef MA__NUMA
	/* Initialize per-node data */
	if (marcel_topo_node_level)
		for (level = &marcel_topo_node_level[0];
		     !marcel_vpset_iszero(&level->vpset); level++)
			level->nodedata = (struct marcel_topo_nodedata)
				MARCEL_TOPO_NODEDATA_INITIALIZER(&level->nodedata);
#  endif

	/* Initialize level value */
	for (l = 0; l < marcel_topo_nblevels; l++)
		for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[l][i].vpset); i++)
			marcel_topo_levels[l][i].level = l;

#  ifdef MA__NUMA
	if (marcel_topo_node_level)
		/* Migrate data to respective memory nodes */
		for (l = 0; l < marcel_topo_nblevels; l++)
			for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[l][i].vpset);
			     i++) {
				for (level = &marcel_topo_levels[l][i];
				     level && level->type != MARCEL_LEVEL_NODE;
				     level = level->father);
				if (level)
					/* That's not a level above NUMA nodes */
					ma_migrate_mem(&marcel_topo_levels[l][i],
						       sizeof(marcel_topo_levels[l][i]) -
						       sizeof(marcel_topo_levels[l]
							      [i].pad), level->os_node);
			}
#  endif

#  ifdef MA__NUMA
	if (marcel_topo_node_level) {
		unsigned int node, marcel_nbnodes_aux = 0;
		for (node = 0; node < marcel_nbnodes; node++)
			if (!marcel_vpset_iszero(&marcel_topo_node_level[node].cpuset)) {
				marcel_nbnodes_aux++;
				MARCEL_LOG("Node %d is enabled\n", node);
			}
		if (marcel_nbnodes_aux != marcel_nbnodes) {
			MARCEL_LOG("Fixing number of nodes from %d to %d\n",
				   marcel_nbnodes, marcel_nbnodes_aux);
			marcel_nbnodes = marcel_nbnodes_aux;
		}
	}
#    ifndef MARCEL_SMT_ENABLED
	if (nb_physical_processors < marcel_nbvps()) {
		MA_WARN_USER("several VPs are used on the same core, but Marcel was not "
			     "configured with --enable-smt to optimize core sleep !\n");
	}
#    endif
#  endif

}

void ma_topo_exit(void)
{
	unsigned l, i;
	/* Last level is not freed because we need it in various places */
	for (l = 0; l < marcel_topo_nblevels - 1; l++) {
		for (i = 0; !marcel_vpset_iszero(&marcel_topo_levels[l][i].cpuset); i++) {
			marcel_free(marcel_topo_levels[l][i].children);
			marcel_topo_levels[l][i].children = NULL;
		}
		if (l) {
			marcel_free(marcel_topo_levels[l]);
			marcel_topo_levels[l] = NULL;
		}
	}
}


/* Topology initialization.  */

static void initialize_topology(void)
{
	topo_discover();

	/* Now we can set VP levels for main LWP */
	ma_init_lwp_vpnum(0, &__main_lwp);
}

__ma_initfunc(initialize_topology, MA_INIT_TOPOLOGY, "Finding Topology");


void ma_disable_topology_vps(const marcel_vpset_t * vpset)
{
	int vp;
	struct marcel_topo_level *l, *father;
	unsigned i;
	marcel_vpset_foreach_begin(vp, vpset)
		/* Browse up to disable the VP in vpsets, and unlink levels
		 * with empty vpsets */
		for (l = &marcel_topo_vp_level[vp]; l; l = l->father) {
			marcel_vpset_clr(&l->vpset, vp);
			father = l->father;
			if (father && marcel_vpset_iszero(&l->vpset)) {
				for (i = 0; i < father->arity; i++) {
					if (father->children[i] == l) {
						father->arity--;
						memmove(&father->children[i],
							&father->children[i + 1],
							(father->arity -
							 i) * sizeof(father->children[0]));
					}
				}
			}
		}
	marcel_vpset_foreach_end(vp, vpset)
		}

void ma_enable_topology_vps(const marcel_vpset_t * vpset)
{
	int vp;
	struct marcel_topo_level *l, *father;
	unsigned i;
	marcel_vpset_foreach_begin(vp, vpset)
		/* Browse up to enable the VP in vpsets, and relink levels */
		for (l = &marcel_topo_vp_level[vp]; l; l = l->father) {
			marcel_vpset_set(&l->vpset, vp);
			father = l->father;
			if (father) {
				for (i = 0; i < father->arity; i++) {
					if (father->children[i] == l)
						/* Already there. */
						break;
					/* Put it at the right place */
					if (father->children[i] != l
					    && marcel_vpset_first(&l->vpset) <
					    marcel_vpset_first(&father->children[i]->vpset)) {
						memmove(&father->children[i + 1],
							&father->children[i],
							(father->arity -
							 i) * sizeof(father->children[0]));
						father->arity++;
						father->children[i] = l;
						break;
					}
				}
				if (i == father->arity) {
					father->arity++;
					father->children[i] = l;
				}
			}
		}
	marcel_vpset_foreach_end(vp, vpset)
		}

#endif				/* MA__LWPS */
