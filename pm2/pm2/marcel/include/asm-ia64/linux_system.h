//#ifndef _ASM_IA64_SYSTEM_H
//#define _ASM_IA64_SYSTEM_H

/*
 * System defines. Note that this is included both from .c and .S
 * files, so it does only defines, not any C code.  This is based
 * on information published in the Processor Abstraction Layer
 * and the System Abstraction Layer manual.
 *
 * Copyright (C) 1998-2001 Hewlett-Packard Co
 * Copyright (C) 1998-2001 David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */
// #include <linux/config.h>


// #include <asm/page.h>



#section common
/*
 * Similar to:
 * include/asm-ia64/system.h
 */

#section marcel_macros


#define MA_KERNEL_START		(PAGE_OFFSET + 68*1024*1024)

/*
 * The following #defines must match with vmlinux.lds.S:
 */
#define MA_IVT_ADDR		(KERNEL_START)
#define MA_IVT_END_ADDR		(KERNEL_START + 0x8000)
#define MA_ZERO_PAGE_ADDR		PAGE_ALIGN(IVT_END_ADDR)
#define MA_SWAPPER_PGD_ADDR	(ZERO_PAGE_ADDR + 1*PAGE_SIZE)

#define MA_GATE_ADDR		(0xa000000000000000 + PAGE_SIZE)
#define MA_PERCPU_ADDR		(0xa000000000000000 + 2*PAGE_SIZE)

#if defined(CONFIG_ITANIUM_B0_SPECIFIC) || defined(CONFIG_ITANIUM_B1_SPECIFIC)
  /* Workaround for Errata 97.  */
# define MA_IA64_SEMFIX_INSN	mf;
# define MA_IA64_SEMFIX	"mf;"
#else
# define MA_IA64_SEMFIX_INSN
# define MA_IA64_SEMFIX	""
#endif



//#ifndef __ASSEMBLY__

//#include <linux/kernel.h>
//#include <linux/types.h>


#section marcel_structures


struct ma_pci_vector_struct {
	__ma_u16 bus;	/* PCI Bus number */
	__ma_u32 pci_id;	/* ACPI split 16 bits device, 16 bits function (see section 6.1.1) */
	__ma_u8 pin;	/* PCI PIN (0 = A, 1 = B, 2 = C, 3 = D) */
	__ma_u8 irq;	/* IRQ assigned */
};


extern struct ma_ia64_boot_param {
	__ma_u64 command_line;		/* physical address of command line arguments */
	__ma_u64 efi_systab;		/* physical address of EFI system table */
	__ma_u64 efi_memmap;		/* physical address of EFI memory map */
	__ma_u64 efi_memmap_size;		/* size of EFI memory map */
	__ma_u64 efi_memdesc_size;		/* size of an EFI memory map descriptor */
	__ma_u32 efi_memdesc_version;	/* memory descriptor version */
	struct {
		__ma_u16 num_cols;	/* number of columns on console output device */
		__ma_u16 num_rows;	/* number of rows on console output device */
		__ma_u16 orig_x;	/* cursor's x position */
		__ma_u16 orig_y;	/* cursor's y position */
	} console_info;
	__ma_u64 fpswa;		/* physical address of the fpswa interface */
	__ma_u64 initrd_start;
	__ma_u64 initrd_size;
} *ma_ia64_boot_param;

#section marcel_functions

static inline void
ma_ia64_insn_group_barrier (void);

#section marcel_inline

static inline void
ma_ia64_insn_group_barrier (void)
{
	__asm__ __volatile__ (";;" ::: "memory");
}

#section marcel_macros
/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   rmb():	Like wmb(), but for reads.
 *   mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */
#define ma_mb()	__asm__ __volatile__ ("mf" ::: "memory")
#define ma_rmb()	ma_mb()
#define ma_wmb()	ma_mb()

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
#endif

/*
 * XXX check on these---I suspect what Linus really wants here is
 * acquire vs release semantics but we can't discuss this stuff with
 * Linus just yet.  Grrr...
 */
#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_mb(); } while (0)

/*
 * The group barrier in front of the rsm & ssm are necessary to ensure
 * that none of the previous instructions in the same group are
 * affected by the rsm/ssm.
 */
/* For spinlocks etc */
#if 0 
// on saute le debug irq

#ifdef CONFIG_IA64_DEBUG_IRQ

  extern unsigned long last_cli_ip;

# define local_irq_save(x)								\
do {											\
	unsigned long ip, psr;								\
											\
	__asm__ __volatile__ ("mov %0=psr;; rsm psr.i;;" : "=r" (psr) :: "memory");	\
	if (psr & (1UL << 14)) {							\
		__asm__ ("mov %0=ip" : "=r"(ip));					\
		last_cli_ip = ip;							\
	}										\
	(x) = psr;									\
} while (0)

# define local_irq_disable()								\
do {											\
	unsigned long ip, psr;								\
											\
	__asm__ __volatile__ ("mov %0=psr;; rsm psr.i;;" : "=r" (psr) :: "memory");	\
	if (psr & (1UL << 14)) {							\
		__asm__ ("mov %0=ip" : "=r"(ip));					\
		last_cli_ip = ip;							\
	}										\
} while (0)

# define local_irq_restore(x)						 \
do {									 \
	unsigned long ip, old_psr, psr = (x);				 \
									 \
	__asm__ __volatile__ (";;mov %0=psr; mov psr.l=%1;; srlz.d"	 \
			      : "=&r" (old_psr) : "r" (psr) : "memory"); \
	if ((old_psr & (1UL << 14)) && !(psr & (1UL << 14))) {		 \
		__asm__ ("mov %0=ip" : "=r"(ip));			 \
		last_cli_ip = ip;					 \
	}								 \
} while (0)

#else /* !CONFIG_IA64_DEBUG_IRQ */
  /* clearing of psr.i is implicitly serialized (visible by next insn) */
# define local_irq_save(x)	__asm__ __volatile__ ("mov %0=psr;; rsm psr.i;;"	\
						      : "=r" (x) :: "memory")
# define local_irq_disable()	__asm__ __volatile__ (";; rsm psr.i;;" ::: "memory")
/* (potentially) setting psr.i requires data serialization: */
# define local_irq_restore(x)	__asm__ __volatile__ (";; mov psr.l=%0;; srlz.d"	\
						      :: "r" (x) : "memory")
#endif /* !CONFIG_IA64_DEBUG_IRQ */

#endif // ! on saute le debug irq */

#define ma_local_irq_enable()	__asm__ __volatile__ (";; ssm psr.i;; srlz.d" ::: "memory")

#define __ma_cli()			ma_local_irq_disable ()
#define __ma_save_flags(flags)	__asm__ __volatile__ ("mov %0=psr" : "=r" (flags) :: "memory")
#define __ma_save_and_cli(flags)	ma_local_irq_save(flags)
#define ma_save_and_cli(flags)	__ma_save_and_cli(flags)
#define __ma_sti()			ma_local_irq_enable ()
#define __ma_restore_flags(flags)	ma_local_irq_restore(flags)

#ifdef MA__LWPS
  extern void __ma_global_cli (void);
  extern void __ma_global_sti (void);
  extern unsigned long __ma_global_save_flags (void);
  extern void __ma_global_restore_flags (unsigned long);
# define ma_cli()			__ma_global_cli()
# define ma_sti()			__ma_global_sti()
# define ma_save_flags(flags)	((flags) = __ma_global_save_flags())
# define ma_restore_flags(flags)	__ma_global_restore_flags(flags)
#else /* !CONFIG_SMP */
# define ma_cli()			__ma_cli()
# define ma_sti()			__ma_sti()
# define ma_save_flags(flags)	__ma_save_flags(flags)
# define ma_restore_flags(flags)	__ma_restore_flags(flags)
#endif /* !CONFIG_SMP */

/*
 * Force an unresolved reference if someone tries to use
 * ia64_fetch_and_add() with a bad value.
 */
extern unsigned long __ma_bad_size_for_ia64_fetch_and_add (void);
extern unsigned long __ma_bad_increment_for_ia64_fetch_and_add (void);

#define MA_IA64_FETCHADD(tmp,v,n,sz)						\
({										\
	switch (sz) {								\
	      case 4:								\
		__asm__ __volatile__ (MA_IA64_SEMFIX"fetchadd4.rel %0=[%1],%2"	\
				      : "=r"(tmp) : "r"(v), "i"(n) : "memory");	\
		break;								\
										\
	      case 8:								\
		__asm__ __volatile__ (MA_IA64_SEMFIX"fetchadd8.rel %0=[%1],%2"	\
				      : "=r"(tmp) : "r"(v), "i"(n) : "memory");	\
		break;								\
										\
	      default:								\
		__ma_bad_size_for_ia64_fetch_and_add();				\
	}									\
})

#define ma_ia64_fetch_and_add(i,v)							\
({										\
	__ma_u64 _tmp;								\
	volatile __typeof__(*(v)) *_v = (v);					\
	switch (i) {								\
	      case -16:	MA_IA64_FETCHADD(_tmp, _v, -16, sizeof(*(v))); break;	\
	      case  -8: MA_IA64_FETCHADD(_tmp, _v,  -8, sizeof(*(v))); break;	\
	      case  -4:	MA_IA64_FETCHADD(_tmp, _v,  -4, sizeof(*(v))); break;	\
	      case  -1:	MA_IA64_FETCHADD(_tmp, _v,  -1, sizeof(*(v))); break;	\
	      case   1:	MA_IA64_FETCHADD(_tmp, _v,   1, sizeof(*(v))); break;	\
	      case   4: MA_IA64_FETCHADD(_tmp, _v,   4, sizeof(*(v))); break;	\
	      case   8:	MA_IA64_FETCHADD(_tmp, _v,   8, sizeof(*(v))); break;	\
	      case  16:	MA_IA64_FETCHADD(_tmp, _v,  16, sizeof(*(v))); break;	\
	      default:								\
		_tmp = __ma_bad_increment_for_ia64_fetch_and_add();		\
		break;								\
	}									\
	(__typeof__(*v)) (_tmp + (i));	/* return new value */			\
})

/*
 * This function doesn't exist, so you'll get a linker error if
 * something tries to do an invalid xchg().
 */
extern void __ma_xchg_called_with_bad_pointer (void);


#section marcel_functions

static __inline__ unsigned long
__ma_xchg (unsigned long x, volatile void *ptr, int size);

#section marcel_inline
static __inline__ unsigned long
__ma_xchg (unsigned long x, volatile void *ptr, int size)
{
	unsigned long result;

	switch (size) {
	      case 1:
		__asm__ __volatile (MA_IA64_SEMFIX"xchg1 %0=[%1],%2" : "=r" (result)
				    : "r" (ptr), "r" (x) : "memory");
		return result;

	      case 2:
		__asm__ __volatile (MA_IA64_SEMFIX"xchg2 %0=[%1],%2" : "=r" (result)
				    : "r" (ptr), "r" (x) : "memory");
		return result;

	      case 4:
		__asm__ __volatile (MA_IA64_SEMFIX"xchg4 %0=[%1],%2" : "=r" (result)
				    : "r" (ptr), "r" (x) : "memory");
		return result;

	      case 8:
		__asm__ __volatile (MA_IA64_SEMFIX"xchg8 %0=[%1],%2" : "=r" (result)
				    : "r" (ptr), "r" (x) : "memory");
		return result;
	}
	__ma_xchg_called_with_bad_pointer();
	return x;
}

#section marcel_macros

#define ma_xchg(ptr,x)							     \
  ((__typeof__(*(ptr))) __ma_xchg ((unsigned long) (x), (ptr), sizeof(*(ptr))))

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */

#define __MA_HAVE_ARCH_CMPXCHG 1

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid cmpxchg().
 */
extern long __ma_cmpxchg_called_with_bad_pointer(void);

#define ma_ia64_cmpxchg(sem,ptr,old,new,size)						\
({											\
	__typeof__(ptr) _p_ = (ptr);							\
	__typeof__(new) _n_ = (new);							\
	__ma_u64 _o_, _r_;									\
											\
	switch (size) {									\
	      case 1: _o_ = (__ma_u8 ) (long) (old); break;				\
	      case 2: _o_ = (__ma_u16) (long) (old); break;				\
	      case 4: _o_ = (__ma_u32) (long) (old); break;				\
	      case 8: _o_ = (__ma_u64) (long) (old); break;				\
	      default: break;								\
	}										\
	 __asm__ __volatile__ ("mov ar.ccv=%0;;" :: "rO"(_o_));				\
	switch (size) {									\
	       case 1:									\
		__asm__ __volatile__ (MA_IA64_SEMFIX"cmpxchg1."sem" %0=[%1],%2,ar.ccv"	\
				      : "=r"(_r_) : "r"(_p_), "r"(_n_) : "memory");	\
		break;									\
											\
	      case 2:									\
		__asm__ __volatile__ (MA_IA64_SEMFIX"cmpxchg2."sem" %0=[%1],%2,ar.ccv"	\
				      : "=r"(_r_) : "r"(_p_), "r"(_n_) : "memory");	\
		break;									\
											\
	      case 4:									\
		__asm__ __volatile__ (MA_IA64_SEMFIX"cmpxchg4."sem" %0=[%1],%2,ar.ccv"	\
				      : "=r"(_r_) : "r"(_p_), "r"(_n_) : "memory");	\
		break;									\
											\
	      case 8:									\
		__asm__ __volatile__ (MA_IA64_SEMFIX"cmpxchg8."sem" %0=[%1],%2,ar.ccv"	\
				      : "=r"(_r_) : "r"(_p_), "r"(_n_) : "memory");	\
		break;									\
											\
	      default:									\
		_r_ = __ma_cmpxchg_called_with_bad_pointer();				\
		break;									\
	}										\
	(__typeof__(old)) _r_;								\
})

#define ma_cmpxchg_acq(ptr,o,n)	ma_ia64_cmpxchg("acq", (ptr), (o), (n), sizeof(*(ptr)))
#define ma_cmpxchg_rel(ptr,o,n)	ma_ia64_cmpxchg("rel", (ptr), (o), (n), sizeof(*(ptr)))

/* for compatibility with other platforms: */
#define ma_cmpxchg(ptr,o,n)	ma_cmpxchg_acq(ptr,o,n)

#ifdef MA_CONFIG_IA64_DEBUG_CMPXCHG
# define MA_CMPXCHG_BUGCHECK_DECL	int _ma_cmpxchg_bugcheck_count = 128;
# define MA_CMPXCHG_BUGCHECK(v)							\
  do {										\
	if (_ma_cmpxchg_bugcheck_count-- <= 0) {					\
		void *ip;							\
		extern int printk(const char *fmt, ...);			\
		asm ("mov %0=ip" : "=r"(ip));					\
		printk("CMPXCHG_BUGCHECK: stuck at %p on word %p\n", ip, (v));	\
		break;								\
	}									\
  } while (0)
#else /* !CONFIG_IA64_DEBUG_CMPXCHG */
# define MA_CMPXCHG_BUGCHECK_DECL
# define MA_CMPXCHG_BUGCHECK(v)
#endif /* !CONFIG_IA64_DEBUG_CMPXCHG */

//#ifdef __KERNEL__

#define prepare_to_switch()    do { } while(0)

#ifdef CONFIG_IA32_SUPPORT
# define MA_IS_IA32_PROCESS(regs)	(ia64_psr(regs)->is != 0)
#else
# define MA_IS_IA32_PROCESS(regs)		0
#endif

/*
 * Context switch from one thread to another.  If the two threads have
 * different address spaces, schedule() has already taken care of
 * switching to the new address space by calling switch_mm().
 *
 * Disabling access to the fph partition and the debug-register
 * context switch MUST be done before calling ia64_switch_to() since a
 * newly created thread returns directly to
 * ia64_ret_from_syscall_clear_r8.
 */
extern struct ma_task_struct *ia64_switch_to (void *next_task);

extern void ma_ia64_save_extra (struct ma_task_struct *task);
extern void ma_ia64_load_extra (struct ma_task_struct *task);

#define __ma_switch_to(prev,next,last) do {						\
	if (((prev)->thread.flags & (MA_IA64_THREAD_DBG_VALID|MA_IA64_THREAD_PM_VALID))	\
	    || MA_IS_IA32_PROCESS(ia64_task_regs(prev)))					\
		ma_ia64_save_extra(prev);							\
	if (((next)->thread.flags & (MA_IA64_THREAD_DBG_VALID|MA_IA64_THREAD_PM_VALID))	\
	    || MA_IS_IA32_PROCESS(ia64_task_regs(next)))					\
		ma_ia64_load_extra(next);							\
	(last) = ma_ia64_switch_to((next));						\
} while (0)

#ifdef MA__LWPS
  /*
   * In the SMP case, we save the fph state when context-switching
   * away from a thread that modified fph.  This way, when the thread
   * gets scheduled on another CPU, the CPU can pick up the state from
   * task->thread.fph, avoiding the complication of having to fetch
   * the latest fph state from another CPU.
   */
# define ma_switch_to(prev,next,last) do {						\
	if (ia64_psr(ia64_task_regs(prev))->mfh) {				\
		ia64_psr(ia64_task_regs(prev))->mfh = 0;			\
		(prev)->thread.flags |= MA_IA64_THREAD_FPH_VALID;			\
		__ia64_save_fpu((prev)->thread.fph);				\
	}									\
	ia64_psr(ia64_task_regs(prev))->dfh = 1;				\
	__ma_switch_to(prev,next,last);						\
  } while (0)
#else
# define ma_switch_to(prev,next,last) do {						\
	ia64_psr(ia64_task_regs(next))->dfh = (ia64_get_fpu_owner() != (next));	\
	__ma_switch_to(prev,next,last);						\
} while (0)
#endif

//#endif /* __KERNEL__ */

//#endif /* __ASSEMBLY__ */

//#endif /* _ASM_IA64_SYSTEM_H */
