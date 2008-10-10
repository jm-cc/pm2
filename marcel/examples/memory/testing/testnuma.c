/* ne *pas* mettre de -O, sinon gcc arrive à éliminer les latences (et puis il optimise mes opérations débiles */
//#define _AIX
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#if defined(__osf__) || defined(__linux__)
#define NUMA_VERSION1_COMPATIBILITY
#include <numa.h>
#endif
#ifdef _AIX
#include <sys/rset.h>
#include <sys/malloc.h>
#endif
#include <sys/mman.h>
#include <stdarg.h>
#include <signal.h>
#include <sched.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <inttypes.h>

#ifdef __x86_64__
#define use_movnt
#endif
//#define use_movnt

static inline int max(int x, int y) { if (x<y) return y; else return x; }
static inline int min(int x, int y) { if (x>y) return y; else return x; }

int argh(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
	exit(1);
	//abort();
}

int argherrno(char *fun) {
	fprintf(stderr,"error %d\n",errno);
	perror(fun);
	exit(1);
	//abort();
}

#define MEMSHIFT (4+20) /* more than cache size */
#define STARTSHIFT (10)
#define MEMPLUS 2
#define MEMSIZE (1<<MEMSHIFT)
#define STARTSIZE (1<<STARTSHIFT)

#define LINESIZE 1

#define NODE_ALLOC 0
#define EXP_REF 0
#if 0
#define TABLEAUEXPS
int testcpus[] = {0,0,0,1,1,1,0,0,0};
#define NUMEXPS (sizeof(testcpus)/sizeof(*testcpus))
#else
#define NUMEXPS totcpus
#endif
#define NLOOPS 4

unsigned long (*loopl)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*loopd)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*loop2l)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*loop2d)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*breadtimes)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*bwritetimes)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*readtimes)[MEMSHIFT-STARTSHIFT+1];
unsigned long (*writetimes)[MEMSHIFT-STARTSHIFT+1];
#ifdef use_movnt
typedef unsigned long long tbx_tick_t, *p_tbx_tick_t;
#define TBX_GET_TICK(t) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (t) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
} while(0)
unsigned long (*movnt);
unsigned long (*prefetchflush);
#endif
unsigned int *testedcpu;
unsigned int *testednode;
int *nodecpus;

int lastnode;
void handle(int foo) { }

int numa;

int marcel_main(void) {
	int node,prevnode,mems,k,m,cpu,prevcpu;
	long mem, free;
	volatile char *c;
	int nbcpus,totcpus=0;
	char sum;
	struct timeval tv1,tv2;
	unsigned int gold;
	int exp;
#ifdef __linux__
	cpu_set_t cpus;
	cpu_set_t cpusbind;
#endif
#ifdef __osf__
	cpuset_t cpuset;
	radset_t radset;
	rad_info_t radinfo;
	cpu_cursor_t cursor;
	memalloc_attr_t mattr;
	void *ap;
	cpuid_t cpuid;
#endif
#ifdef _AIX
	rsethandle_t rset, rad;
	int MCMlevel = rs_getinfo(NULL, R_MCMSDL, 0);
	fprintf(stderr,"smp level %d\n",rs_getinfo(NULL,R_SMPSDL,0));
	fprintf(stderr,"memory level %d\n",MCMlevel);
	fprintf(stderr,"l2 level %d\n",rs_getinfo(NULL,R_L2CSDL,0));
	fprintf(stderr,"core level %d\n",rs_getinfo(NULL,R_PCORESDL,0));
	fprintf(stderr,"max level %d\n",rs_getinfo(NULL,R_MAXSDL,0));
#endif

	signal(SIGWINCH,handle);

#ifdef __linux__
	if (!(numa = (numa_available()>=0)))
		fprintf(stderr,"numa not available\n");
	else {
		numa_set_bind_policy(1);
		numa_set_strict(1);
	}
#endif
#ifdef __osf__
	lastnode = rad_get_num()-1;
#endif
#ifdef __linux__
	if (numa)
		lastnode = numa_max_node();
#endif
#ifdef _AIX
	rset = rs_alloc(RS_PARTITION);
	rad = rs_alloc(RS_EMPTY);
	lastnode = rs_numrads(rset, MCMlevel, 0)-1;
	if (lastnode == -2) {
		perror("rs_numrads");
		exit(1);
	}
	if (lastnode)
		numa = 1;
#endif
	printf("%d node%s\n", lastnode + 1, lastnode?"s":"");

#ifdef __osf__
	cpusetcreate(&cpuset);
	radsetcreate(&radset);
	radinfo.rinfo_version = RAD_INFO_VERSION;
	cpusetcreate(&radinfo.rinfo_cpuset);
#endif

	printf("%d pagesize\n", getpagesize());
	nodecpus = malloc((lastnode+1)*sizeof(*nodecpus));
	if (numa)
	for (node=0;node<=lastnode;node++) {
		printf("node %d:",node);
#ifdef __osf__
		cpuemptyset(radinfo.rinfo_cpuset);
		if (rad_get_info(node,&radinfo)<0)
			argherrno("rad_get_info");
		printf("id %d state %d\n", radinfo.rinfo_radid, radinfo.rinfo_state);
#endif
#ifdef __linux__
		CPU_ZERO(&cpus);
		if (numa_node_to_cpus(node, (unsigned long *) &cpus, sizeof(cpus)))
			argh("node_to_cpus");
#endif
#ifdef __osf__
		nbcpus = cpucountset(radinfo.rinfo_cpuset);
#endif
#ifdef __linux__
		for (nbcpus=0,cpu=0;cpu<CPU_SETSIZE;cpu++)
			if (CPU_ISSET(cpu, &cpus)) {
				printf(" %d", cpu);
				nbcpus++;
			}
#endif
#ifdef _AIX
		rs_getrad(rset, rad, MCMlevel, node, 0);
		nbcpus = rs_getinfo(rad, R_NUMPROCS, 0);
#endif
		printf("\n -> %d CPU%s\n", nbcpus, nbcpus<=1?"":"s");
#ifdef __osf__
		mem = radinfo.rinfo_physmem * getpagesize();
		free = radinfo.rinfo_freemem * getpagesize();
#endif
#ifdef __linux__
		if ((mem = numa_node_size(node,&free))<0)
			argherrno("numa_node_size");
#endif
#ifdef _AIX
		mem = rs_getinfo(rad, R_NUMMEMPS, 0);
		free = rs_getinfo(rad, R_MEMSIZE, 0);
#endif
		printf("  %ld MB Mem (%ld free)\n", mem>>20, free>>20);
		nodecpus[node]=nbcpus;
		totcpus+=nbcpus;
	}
	else {
		lastnode = 0;
		totcpus = 1;
	}
	if (!lastnode)
		printf("not much to test with only one node\n");

	breadtimes = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**breadtimes));
	bwritetimes = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**bwritetimes));
	readtimes = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**readtimes));
	writetimes = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**writetimes));
	loopl = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**loopl));
	loopd = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**loopd));
	loop2l = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**loop2l));
	loop2d = malloc(NUMEXPS*(MEMSHIFT-STARTSHIFT+1)*sizeof(**loop2d));
#ifdef use_movnt
	movnt = malloc(NUMEXPS*sizeof(*movnt));
	prefetchflush = malloc(NUMEXPS*sizeof(*prefetchflush));
#endif
	testedcpu = malloc(NUMEXPS*sizeof(*testedcpu));
	testednode = malloc(NUMEXPS*sizeof(*testednode));

	printf("allocating on node %d, exp reference %d\n",NODE_ALLOC,EXP_REF);
#ifdef __osf__
	memset(&mattr, 0, sizeof(mattr));
	mattr.mattr_policy = MPOL_DIRECTED;
	if (!VALIDATE_POLICY(mattr.mattr_policy))
		fprintf(stderr,"pas valide\n");
	mattr.mattr_rad = NODE_ALLOC;
	radsetcreate(&mattr.mattr_radset);
	rademptyset(mattr.mattr_radset);
	radaddset(mattr.mattr_radset,NODE_ALLOC);
	if ((c = nmmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, &mattr)) == (void*)-1)
		argherrno("nmmap");
#endif
#ifdef __linux__
	if (numa) {
		c = numa_alloc_onnode(MEMSIZE, NODE_ALLOC);
		printf("c is at %p\n",c);
	} else
		c = malloc(MEMSIZE);
#endif
#ifdef _AIX
	rsid_t rsid = {.at_rset = rad};
	rs_getrad(rset, rad, MCMlevel, NODE_ALLOC, 0);
	c = ra_mmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, R_RSET, rsid, P_DEFAULT);
	fprintf(stderr,"return %p\n",c);
	perror("mmap");
#endif
	memset(c,0,MEMSIZE);

	exp = -1;
	/************************/
	/* boucle sur les n½uds */
	/************************/
	for (node=0;node<=lastnode;node++) {
		printf("node %d\n",node);
#ifdef __osf__
	/* refaire l'allocation: apparemment osf fait migrer les pages ... */
		munmap(c,MEMSIZE);
		if ((c = nmmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, &mattr)) == (void*)-1)
			argherrno("nmmap");
		memset(c,0,MEMSIZE);

		cpuemptyset(cpuset);
		if (rad_get_cpus(node,cpuset))
#endif
#ifdef __linux__
		if (numa)
		if (numa_node_to_cpus(node, (unsigned long *) &cpus, sizeof(cpus)))
#endif
#ifdef _AIX
		if (rs_getrad(rset, rad, MCMlevel, node, 0))
#endif
			argh("node_to_cpus");


		/***********************/
		/* boucle sur les cpus */
		/***********************/
#ifdef __osf__
		cursor = SET_CURSOR_INIT;
		while ((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE/* && cpuid<2*/) {
			cpu = cpuid;
			rademptyset(radset);
			radaddset(radset,cpu);
/*
			if (rad_bind_pid(NULL,radset,RAD_INSIST))
					argherrno("rad_bind_pid");
*/
			if (pthread_rad_attach(pthread_self(), radset, RAD_INSIST))
				argherrno("pthread_rad_attach");
			/*pthread_use_only_cpu(pthread_self(), cpu, 0);*/

#endif



#ifdef __linux__
#ifdef TABLEAUEXPS
			int *thecpu;
			for (thecpu=testcpus;thecpu<testcpus+NUMEXPS;thecpu++) {
				cpu = *thecpu;
#else
		for (cpu=0;cpu<totcpus;cpu++) {
#endif
			if (numa)
				if (!CPU_ISSET(cpu, &cpus))
					continue;

			CPU_ZERO(&cpusbind);
			CPU_SET(cpu,&cpusbind);
#if 1
			if (sched_setaffinity(0, sizeof(cpusbind), &cpusbind))
#else
			if (sched_setaffinity(0, &cpusbind))
#endif
			{
				fprintf(stderr,"cpu %d:\n  ",cpu);
				argherrno("sched_setaffinity");
			}
#endif
#ifdef _AIX
		for (cpu=0;cpu<totcpus;cpu++) {
			if (!rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, cpu))
				continue;
			bindprocessor(BINDTHREAD, thread_self(), cpu);
#endif

			printf("exp %d cpu %d",++exp,cpu);
			testedcpu[exp] = cpu;
			testednode[exp] = node;

			/*******************************/
			/* boucle sur la taille buffer */
			/*******************************/
			memset(c,0,MEMSIZE);
			memcpy(c,c,MEMSIZE);
#define TEMPS	((tv2.tv_sec-tv1.tv_sec)*1000000UL+(tv2.tv_usec-tv1.tv_usec))
#define LATENCE(what) \
	what[exp][mems-STARTSHIFT] = (TEMPS*1000UL*LINESIZE)>>(mems)
#define DEBIT(what) \
	what[exp][mems-STARTSHIFT] = (mem/LINESIZE)/TEMPS

#define PRELOOP for (m=0;m<NLOOPS;m++)
	//for (m=0;m<max(1,min((1<<20)/mem,32));m++)
#define LOOP for (k=0;k<mem;k+=LINESIZE)
#ifdef use_movnt
			mems = 18;
			mem = 1<<mems;
			PRELOOP {
				gettimeofday(&tv1,NULL);
				LOOP	{
					 __builtin_ia32_movnti((void*) &c[0], gold);
				}
				gettimeofday(&tv2,NULL);
			}
			movnt[exp] = (TEMPS*1000UL)>>mems;
			printf(" movnt %ld",movnt[exp]);
			tbx_tick_t t1,t2,t3,tt1,tt2;
			PRELOOP {
				gettimeofday(&tv1,NULL);
				TBX_GET_TICK(tt1);
				LOOP	{
					TBX_GET_TICK(t1);
					__builtin_prefetch((void*)&c[0]);
					TBX_GET_TICK(t2);
					__builtin_ia32_clflush((void*)&c[0]);
					TBX_GET_TICK(t3);
				}
				TBX_GET_TICK(tt2);
				gettimeofday(&tv2,NULL);
			}
			__builtin_ia32_emms();
			prefetchflush[exp] = (TEMPS*1000UL)>>mems;
			printf(" preflush %ld",prefetchflush[exp]);
			printf(" pre %lld flush %lld",t2-t1,t3-t2);
			printf(" loop %lld",(tt2-tt1)>>mems);
			printf(" end %lld",(tt2-t3));
#else
			for (mems=STARTSHIFT,mem=1<<mems;mems<=MEMSHIFT;mems+=MEMPLUS,mem<<=MEMPLUS) {
				gold = ((float)mem*((sqrt(5)-1.)/2.));
				printf(" %d",mems);
				fflush(stdout);
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP ;
					gettimeofday(&tv2,NULL);
					LATENCE(loopl);
					DEBIT(loopd);
				}
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP sum+=(k*gold)%mem;
					gettimeofday(&tv2,NULL);
					LATENCE(loop2l);
					DEBIT(loop2d);
				}
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP sum+=c[k];
					gettimeofday(&tv2,NULL);
					DEBIT(breadtimes);
				}
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP c[k]=gold;
					gettimeofday(&tv2,NULL);
					DEBIT(bwritetimes);
				}
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP sum+=c[(k*gold)%mem];
					gettimeofday(&tv2,NULL);
					LATENCE(readtimes);
				}
				printf(":%ld",readtimes[exp][mems-STARTSHIFT]);
				PRELOOP {
					gettimeofday(&tv1,NULL);
					LOOP c[(k*gold)%mem]=gold;
					gettimeofday(&tv2,NULL);
					LATENCE(writetimes);
				}
			}
#endif
			printf("\n");
		}
	}
#define SW "9" // Size
#define LW "4" // Latency
#define FW "3" // Ratio
#define TFW "5" // Ratio
	printf("%"SW"s", "");
	prevnode=-1;
	for (exp=0;exp<NUMEXPS;exp++) {
		node = testednode[exp];
		if (node != prevnode)
			printf(" %"LW"s %-"TFW"u", "node", node);
		else
			printf(" %"LW"s %-"TFW"s", "", "");
		prevnode=node;
	}
	printf("\n%"SW"s", "size");
	prevcpu=-1;
	for (exp=0;exp<NUMEXPS;exp++) {
		cpu = testedcpu[exp];
		if (cpu != prevcpu)
			printf(" %"LW"s %-"TFW"u", "cpu", cpu);
		else
			printf(" %"LW"s %"TFW"s", "", "");
		prevcpu=cpu;
	}
	printf("\n");
#define __STATS(what) \
		printf("%"SW"s", ""); \
		for (exp=0;exp<NUMEXPS;exp++) \
			printf(" %"LW"lu %1.0"FW"f", what[exp], ((float)what[exp])/((float)what[EXP_REF])); \
		printf("\n");
#define STATS(what) \
	for (mems=STARTSHIFT, mem=1<<mems; mems<=MEMSHIFT;mems+=MEMPLUS,mem<<=MEMPLUS) { \
		printf("%"SW"lu", mem); \
		for (exp=0;exp<NUMEXPS;exp++) \
			printf(" %"LW"lu %1.0"FW"f", what[exp][mems-STARTSHIFT], ((float)what[exp][mems-STARTSHIFT])/((float)what[EXP_REF][mems-STARTSHIFT])); \
		printf("\n"); \
	}
#ifdef use_movnt
	printf("movnt\n");
	__STATS(movnt);
	printf("prefetchflush\n");
	__STATS(prefetchflush);
#else
	printf("loop latence\n");
	STATS(loopl);
	printf("loop débit\n");
	STATS(loopd);
	printf("loop2 latence\n");
	STATS(loop2l);
	printf("loop2 débit\n");
	STATS(loop2d);
	printf("bulk read débit\n");
	STATS(breadtimes);
	printf("bulk write débit\n");
	STATS(bwritetimes);
	printf("read latence\n");
	STATS(readtimes);
	printf("write latence\n");
	STATS(writetimes);
#endif
	return 0;
}
