
#ifndef POINTERS_EST_DEF
#define POINTERS_EST_DEF

typedef union {
   char zone[8];
   long bidon;
} pointer;

#define to_pointer(arg, p_addr)  *((void **)p_addr) = arg

#define to_any_t(p_addr) (*((void **)p_addr))

#endif
