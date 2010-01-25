/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __ASM_IA64_LINUX_GCC_INTRIN_H__
#define __ASM_IA64_LINUX_GCC_INTRIN_H__


/*
 * Copyright (C) 2002,2003 Jun Nakajima <jun.nakajima@intel.com>
 * Copyright (C) 2002,2003 Suresh Siddha <suresh.b.siddha@intel.com>
 */


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* define this macro to get some asm stmts included in 'c' files */
#ifndef __INTEL_COMPILER

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define ma_ia64_barrier()	asm volatile ("":::"memory")

#define ma_ia64_stop()	asm volatile (";;"::)

#define ma_ia64_invala_gr(regnum)	asm volatile ("invala.e r%0" :: "i"(regnum))

#define ma_ia64_invala_fr(regnum)	asm volatile ("invala.e f%0" :: "i"(regnum))

#define ma_ia64_setreg(regnum, val)						\
({										\
	switch (regnum) {							\
	    case _MA_IA64_REG_PSR_L:						\
		    asm volatile ("mov psr.l=%0" :: "r"(val) : "memory");	\
		    break;							\
	    case _MA_IA64_REG_AR_KR0 ... _MA_IA64_REG_AR_EC:				\
		    asm volatile ("mov ar%0=%1" ::				\
		    			  "i" (regnum - _MA_IA64_REG_AR_KR0),	\
					  "r"(val): "memory");			\
		    break;							\
	    case _MA_IA64_REG_CR_DCR ... _MA_IA64_REG_CR_LRR1:			\
		    asm volatile ("mov cr%0=%1" ::				\
				          "i" (regnum - _MA_IA64_REG_CR_DCR),	\
					  "r"(val): "memory" );			\
		    break;							\
	    case _MA_IA64_REG_SP:							\
		    asm volatile ("mov r12=%0" ::				\
			    		  "r"(val): "memory");			\
		    break;							\
	    case _MA_IA64_REG_GP:							\
		    asm volatile ("mov gp=%0" :: "r"(val) : "memory");		\
		break;								\
	    default:								\
		    ma_ia64_bad_param_for_setreg();				\
		    break;							\
	}									\
})

#define ma_ia64_getreg(regnum)							\
({										\
	__ma_u64 ia64_intri_res;							\
										\
	switch (regnum) {							\
	case _MA_IA64_REG_GP:							\
		asm volatile ("mov %0=gp" : "=r"(ia64_intri_res));		\
		break;								\
	case _MA_IA64_REG_IP:							\
		asm volatile ("mov %0=ip" : "=r"(ia64_intri_res));		\
		break;								\
	case _MA_IA64_REG_PSR:							\
		asm volatile ("mov %0=psr" : "=r"(ia64_intri_res));		\
		break;								\
	/*case _MA_IA64_REG_TP:	*//* for current() */				\
		/*ia64_intri_res = ma_ia64_r13;					\
		break;*/								\
	case _MA_IA64_REG_AR_KR0 ... _MA_IA64_REG_AR_EC:				\
		asm volatile ("mov %0=ar%1" : "=r" (ia64_intri_res)		\
				      : "i"(regnum - _MA_IA64_REG_AR_KR0));	\
		break;								\
	case _MA_IA64_REG_CR_DCR ... _MA_IA64_REG_CR_LRR1:				\
		asm volatile ("mov %0=cr%1" : "=r" (ia64_intri_res)		\
				      : "i" (regnum - _MA_IA64_REG_CR_DCR));	\
		break;								\
	case _MA_IA64_REG_SP:							\
		asm volatile ("mov %0=sp" : "=r" (ia64_intri_res));		\
		break;								\
	default:								\
		ma_ia64_bad_param_for_getreg();					\
		break;								\
	}									\
	ia64_intri_res;								\
})

#define ma_ia64_hint_pause 0

#define ma_ia64_hint(mode)						\
({								\
	switch (mode) {						\
	case ma_ia64_hint_pause:					\
		asm volatile ("hint @pause" ::: "memory");	\
		break;						\
	}							\
})


/* Integer values for mux1 instruction */
#define ma_ia64_mux1_brcst 0
#define ma_ia64_mux1_mix   8
#define ma_ia64_mux1_shuf  9
#define ma_ia64_mux1_alt  10
#define ma_ia64_mux1_rev  11

#define ma_ia64_mux1(x, mode)							\
({										\
	__ma_u64 ia64_intri_res;							\
										\
	switch (mode) {								\
	case ma_ia64_mux1_brcst:							\
		asm ("mux1 %0=%1,@brcst" : "=r" (ia64_intri_res) : "r" (x));	\
		break;								\
	case ma_ia64_mux1_mix:							\
		asm ("mux1 %0=%1,@mix" : "=r" (ia64_intri_res) : "r" (x));	\
		break;								\
	case ma_ia64_mux1_shuf:							\
		asm ("mux1 %0=%1,@shuf" : "=r" (ia64_intri_res) : "r" (x));	\
		break;								\
	case ma_ia64_mux1_alt:							\
		asm ("mux1 %0=%1,@alt" : "=r" (ia64_intri_res) : "r" (x));	\
		break;								\
	case ma_ia64_mux1_rev:							\
		asm ("mux1 %0=%1,@rev" : "=r" (ia64_intri_res) : "r" (x));	\
		break;								\
	}									\
	ia64_intri_res;								\
})

#define ma_ia64_popcnt(x)						\
({								\
	__ma_u64 ia64_intri_res;					\
	asm ("popcnt %0=%1" : "=r" (ia64_intri_res) : "r" (x));	\
								\
	ia64_intri_res;						\
})

#define ma_ia64_getf_exp(x)					\
({								\
	long ia64_intri_res;					\
								\
	asm ("getf.exp %0=%1" : "=r"(ia64_intri_res) : "f"(x));	\
								\
	ia64_intri_res;						\
})

#define ma_ia64_shrp(a, b, count)								\
({											\
	__ma_u64 ia64_intri_res;								\
	asm ("shrp %0=%1,%2,%3" : "=r"(ia64_intri_res) : "r"(a), "r"(b), "i"(count));	\
	ia64_intri_res;									\
})

#define ma_ia64_ldfs(regnum, x)					\
({								\
	register double __f__ asm ("f"#regnum);			\
	asm volatile ("ldfs %0=[%1]" :"=f"(__f__): "r"(x));	\
})

#define ma_ia64_ldfd(regnum, x)					\
({								\
	register double __f__ asm ("f"#regnum);			\
	asm volatile ("ldfd %0=[%1]" :"=f"(__f__): "r"(x));	\
})

#define ma_ia64_ldfe(regnum, x)					\
({								\
	register double __f__ asm ("f"#regnum);			\
	asm volatile ("ldfe %0=[%1]" :"=f"(__f__): "r"(x));	\
})

#define ma_ia64_ldf8(regnum, x)					\
({								\
	register double __f__ asm ("f"#regnum);			\
	asm volatile ("ldf8 %0=[%1]" :"=f"(__f__): "r"(x));	\
})

#define ma_ia64_ldf_fill(regnum, x)				\
({								\
	register double __f__ asm ("f"#regnum);			\
	asm volatile ("ldf.fill %0=[%1]" :"=f"(__f__): "r"(x));	\
})

#define ma_ia64_stfs(x, regnum)						\
({									\
	register double __f__ asm ("f"#regnum);				\
	asm volatile ("stfs [%0]=%1" :: "r"(x), "f"(__f__) : "memory");	\
})

#define ma_ia64_stfd(x, regnum)						\
({									\
	register double __f__ asm ("f"#regnum);				\
	asm volatile ("stfd [%0]=%1" :: "r"(x), "f"(__f__) : "memory");	\
})

#define ma_ia64_stfe(x, regnum)						\
({									\
	register double __f__ asm ("f"#regnum);				\
	asm volatile ("stfe [%0]=%1" :: "r"(x), "f"(__f__) : "memory");	\
})

#define ma_ia64_stf8(x, regnum)						\
({									\
	register double __f__ asm ("f"#regnum);				\
	asm volatile ("stf8 [%0]=%1" :: "r"(x), "f"(__f__) : "memory");	\
})

#define ma_ia64_stf_spill(x, regnum)						\
({										\
	register double __f__ asm ("f"#regnum);					\
	asm volatile ("stf.spill [%0]=%1" :: "r"(x), "f"(__f__) : "memory");	\
})

#define ma_ia64_fetchadd4_acq(p, inc)						\
({										\
										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("fetchadd4.acq %0=[%1],%2"				\
				: "=r"(ia64_intri_res) : "r"(p), "i" (inc)	\
				: "memory");					\
										\
	ia64_intri_res;								\
})

#define ma_ia64_fetchadd4_rel(p, inc)						\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("fetchadd4.rel %0=[%1],%2"				\
				: "=r"(ia64_intri_res) : "r"(p), "i" (inc)	\
				: "memory");					\
										\
	ia64_intri_res;								\
})

#define ma_ia64_fetchadd8_acq(p, inc)						\
({										\
										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("fetchadd8.acq %0=[%1],%2"				\
				: "=r"(ia64_intri_res) : "r"(p), "i" (inc)	\
				: "memory");					\
										\
	ia64_intri_res;								\
})

#define ma_ia64_fetchadd8_rel(p, inc)						\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("fetchadd8.rel %0=[%1],%2"				\
				: "=r"(ia64_intri_res) : "r"(p), "i" (inc)	\
				: "memory");					\
										\
	ia64_intri_res;								\
})

#define ma_ia64_xchg1(ptr,x)						\
({									\
	__ma_u64 ia64_intri_res;						\
	asm __volatile ("xchg1 %0=[%1],%2" : "=r" (ia64_intri_res)	\
			    : "r" (ptr), "r" (x) : "memory");		\
	ia64_intri_res;							\
})

#define ma_ia64_xchg2(ptr,x)						\
({									\
	__ma_u64 ia64_intri_res;						\
	asm __volatile ("xchg2 %0=[%1],%2" : "=r" (ia64_intri_res)	\
			    : "r" (ptr), "r" (x) : "memory");		\
	ia64_intri_res;							\
})

#define ma_ia64_xchg4(ptr,x)						\
({									\
	__ma_u64 ia64_intri_res;						\
	asm __volatile ("xchg4 %0=[%1],%2" : "=r" (ia64_intri_res)	\
			    : "r" (ptr), "r" (x) : "memory");		\
	ia64_intri_res;							\
})

#define ma_ia64_xchg8(ptr,x)						\
({									\
	__ma_u64 ia64_intri_res;						\
	asm __volatile ("xchg8 %0=[%1],%2" : "=r" (ia64_intri_res)	\
			    : "r" (ptr), "r" (x) : "memory");		\
	ia64_intri_res;							\
})

#define ma_ia64_cmpxchg1_acq(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg1.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg1_rel(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg1.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg2_acq(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg2.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg2_rel(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
											\
	asm volatile ("cmpxchg2.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg4_acq(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg4.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg4_rel(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg4.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg8_acq(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg8.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_cmpxchg8_rel(ptr, repl, old)						\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
											\
	asm volatile ("cmpxchg8.rel %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(repl) : "memory");	\
	ia64_intri_res;									\
})

#define ma_ia64_mf()	asm volatile ("mf" ::: "memory")
#define ma_ia64_mfa()	asm volatile ("mf.a" ::: "memory")

#define ma_ia64_invala() asm volatile ("invala" ::: "memory")

#define ma_ia64_thash(addr)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("thash %0=%1" : "=r"(ia64_intri_res) : "r" (addr));	\
	ia64_intri_res;								\
})

#define ma_ia64_srlz_i()	asm volatile (";; srlz.i ;;" ::: "memory")
#define ma_ia64_srlz_d()	asm volatile (";; srlz.d" ::: "memory");

#ifdef MA_HAVE_SERIALIZE_DIRECTIVE
# define ma_ia64_dv_serialize_data()		asm volatile (".serialize.data");
# define ma_ia64_dv_serialize_instruction()	asm volatile (".serialize.instruction");
#else
# define ma_ia64_dv_serialize_data()
# define ma_ia64_dv_serialize_instruction()
#endif

#define ma_ia64_nop(x)	asm volatile ("nop %0"::"i"(x));

#define ma_ia64_itci(addr)	asm volatile ("itc.i %0;;" :: "r"(addr) : "memory")

#define ma_ia64_itcd(addr)	asm volatile ("itc.d %0;;" :: "r"(addr) : "memory")


#define ma_ia64_itri(trnum, addr) asm volatile ("itr.i itr[%0]=%1"				\
					     :: "r"(trnum), "r"(addr) : "memory")

#define ma_ia64_itrd(trnum, addr) asm volatile ("itr.d dtr[%0]=%1"				\
					     :: "r"(trnum), "r"(addr) : "memory")

#define ma_ia64_tpa(addr)								\
({										\
	__ma_u64 ia64_pa;								\
	asm volatile ("tpa %0 = %1" : "=r"(ia64_pa) : "r"(addr) : "memory");	\
	ia64_pa;								\
})

#define __ma_ia64_set_dbr(index, val)						\
	asm volatile ("mov dbr[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ma_ia64_set_ibr(index, val)						\
	asm volatile ("mov ibr[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ma_ia64_set_pkr(index, val)						\
	asm volatile ("mov pkr[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ma_ia64_set_pmc(index, val)						\
	asm volatile ("mov pmc[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ma_ia64_set_pmd(index, val)						\
	asm volatile ("mov pmd[%0]=%1" :: "r"(index), "r"(val) : "memory")

#define ma_ia64_set_rr(index, val)							\
	asm volatile ("mov rr[%0]=%1" :: "r"(index), "r"(val) : "memory");

#define ma_ia64_get_cpuid(index)								\
({											\
	__ma_u64 ia64_intri_res;								\
	asm volatile ("mov %0=cpuid[%r1]" : "=r"(ia64_intri_res) : "rO"(index));	\
	ia64_intri_res;									\
})

#define __ma_ia64_get_dbr(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=dbr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ma_ia64_get_ibr(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=ibr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ma_ia64_get_pkr(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=pkr[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ma_ia64_get_pmc(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=pmc[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})


#define ma_ia64_get_pmd(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=pmd[%1]" : "=r"(ia64_intri_res) : "r"(index));	\
	ia64_intri_res;								\
})

#define ma_ia64_get_rr(index)							\
({										\
	__ma_u64 ia64_intri_res;							\
	asm volatile ("mov %0=rr[%1]" : "=r"(ia64_intri_res) : "r" (index));	\
	ia64_intri_res;								\
})

#define ma_ia64_fc(addr)	asm volatile ("fc %0" :: "r"(addr) : "memory")


#define ma_ia64_sync_i()	asm volatile (";; sync.i" ::: "memory")

#define ma_ia64_ssm(mask)	asm volatile ("ssm %0":: "i"((mask)) : "memory")
#define ma_ia64_rsm(mask)	asm volatile ("rsm %0":: "i"((mask)) : "memory")
#define ma_ia64_sum(mask)	asm volatile ("sum %0":: "i"((mask)) : "memory")
#define ma_ia64_rum(mask)	asm volatile ("rum %0":: "i"((mask)) : "memory")

#define ma_ia64_ptce(addr)	asm volatile ("ptc.e %0" :: "r"(addr))

#define ma_ia64_ptcga(addr, size)							\
	asm volatile ("ptc.ga %0,%1" :: "r"(addr), "r"(size) : "memory")

#define ma_ia64_ptcl(addr, size)						\
	asm volatile ("ptc.l %0,%1" :: "r"(addr), "r"(size) : "memory")

#define ma_ia64_ptri(addr, size)						\
	asm volatile ("ptr.i %0,%1" :: "r"(addr), "r"(size) : "memory")

#define ma_ia64_ptrd(addr, size)						\
	asm volatile ("ptr.d %0,%1" :: "r"(addr), "r"(size) : "memory")

/* Values for lfhint in ia64_lfetch and ia64_lfetch_fault */

#define ma_ia64_lfhint_none   0
#define ma_ia64_lfhint_nt1    1
#define ma_ia64_lfhint_nt2    2
#define ma_ia64_lfhint_nta    3

#define ma_ia64_lfetch(lfhint, y)					\
({								\
        switch (lfhint) {					\
        case ma_ia64_lfhint_none:					\
                asm volatile ("lfetch [%0]" : : "r"(y));	\
                break;						\
        case ma_ia64_lfhint_nt1:					\
                asm volatile ("lfetch.nt1 [%0]" : : "r"(y));	\
                break;						\
        case ma_ia64_lfhint_nt2:					\
                asm volatile ("lfetch.nt2 [%0]" : : "r"(y));	\
                break;						\
        case ma_ia64_lfhint_nta:					\
                asm volatile ("lfetch.nta [%0]" : : "r"(y));	\
                break;						\
        }							\
})

#define ma_ia64_lfetch_excl(lfhint, y)					\
({									\
        switch (lfhint) {						\
        case ma_ia64_lfhint_none:						\
                asm volatile ("lfetch.excl [%0]" :: "r"(y));		\
                break;							\
        case ma_ia64_lfhint_nt1:						\
                asm volatile ("lfetch.excl.nt1 [%0]" :: "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nt2:						\
                asm volatile ("lfetch.excl.nt2 [%0]" :: "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nta:						\
                asm volatile ("lfetch.excl.nta [%0]" :: "r"(y));	\
                break;							\
        }								\
})

#define ma_ia64_lfetch_fault(lfhint, y)					\
({									\
        switch (lfhint) {						\
        case ma_ia64_lfhint_none:						\
                asm volatile ("lfetch.fault [%0]" : : "r"(y));		\
                break;							\
        case ma_ia64_lfhint_nt1:						\
                asm volatile ("lfetch.fault.nt1 [%0]" : : "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nt2:						\
                asm volatile ("lfetch.fault.nt2 [%0]" : : "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nta:						\
                asm volatile ("lfetch.fault.nta [%0]" : : "r"(y));	\
                break;							\
        }								\
})

#define ma_ia64_lfetch_fault_excl(lfhint, y)				\
({									\
        switch (lfhint) {						\
        case ma_ia64_lfhint_none:						\
                asm volatile ("lfetch.fault.excl [%0]" :: "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nt1:						\
                asm volatile ("lfetch.fault.excl.nt1 [%0]" :: "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nt2:						\
                asm volatile ("lfetch.fault.excl.nt2 [%0]" :: "r"(y));	\
                break;							\
        case ma_ia64_lfhint_nta:						\
                asm volatile ("lfetch.fault.excl.nta [%0]" :: "r"(y));	\
                break;							\
        }								\
})

#define ma_ia64_intrin_local_irq_restore(x)			\
do {								\
	asm volatile ("     cmp.ne p6,p7=%0,r0;;"		\
		      "(p6) ssm psr.i;"				\
		      "(p7) rsm psr.i;;"			\
		      "(p6) srlz.d"				\
		      :: "r"((x)) : "p6", "p7", "memory");	\
} while (0)

#endif /* __INTEL_COMPILER */


/** Internal global variables **/
/* define this macro to get some asm stmts included in 'c' files */
#ifndef __INTEL_COMPILER

//register unsigned long ma_ia64_r13 asm ("r13") __attribute_used__;

#endif /* __INTEL_COMPILER */


/** Internal functions **/
#ifndef __INTEL_COMPILER

extern void ia64_bad_param_for_setreg (void);
extern void ia64_bad_param_for_getreg (void);

#endif /* __INTEL_COMPILER */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_LINUX_GCC_INTRIN_H__ **/
