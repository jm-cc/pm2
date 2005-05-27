#ifndef ATOMIC_H_EST_DEF
#define ATOMIC_H_EST_DEF
#include "tbx_compiler.h"

#define __atomic_fool_gcc(x) (*(struct { int a[100]; } *)x)

typedef int atomic_t;

#define ATOMIC_INIT(i)	        (i)
#define atomic_read(v)		(*(v))
#define atomic_set(v, i)	(*(v) = (i))

extern __tbx_inline__ void atomic_add(atomic_t i, volatile atomic_t * v)
{
        unsigned long temp;
        __asm__ __volatile__(
                "\n1:\t"
                "ldl_l %0,%1\n\t"
                "addl %0,%2,%0\n\t"
                "stl_c %0,%1\n\t"
                "beq %0,1b\n"
                "2:"
                :"=&r" (temp),
                 "=m" (__atomic_fool_gcc(v))
                :"Ir" (i),
                 "m" (__atomic_fool_gcc(v)));
}

extern __tbx_inline__ void atomic_sub(atomic_t i, volatile atomic_t * v)
{
        unsigned long temp;
        __asm__ __volatile__(
                "\n1:\t"
                "ldl_l %0,%1\n\t"
                "subl %0,%2,%0\n\t"
                "stl_c %0,%1\n\t"
                "beq %0,1b\n"
                "2:"
                :"=&r" (temp),
                 "=m" (__atomic_fool_gcc(v))
                :"Ir" (i),
                 "m" (__atomic_fool_gcc(v)));
}

#define atomic_inc(v) atomic_add(1,(v))
#define atomic_dec(v) atomic_sub(1,(v))

#endif /* ATOMIC_H_EST_DEF */
