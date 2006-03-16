SECTIONS {
/*  .rodata : 
    {
      . = ALIGN(8) ;
      __ma_per_lwp_start = . ;
      *( SORT(.ma.per.lwp.offset.*))
      *(.ma.per.lwp.offset)
      __ma_per_lwp_end = . ;
      __ma_per_lwp_size = __ma_per_lwp_end - __ma_per_lwp_start ;
    }
*/
  .data : 
    {
      . = ALIGN(4096) ;
      *(.ma.runqueues)

      . = ALIGN(4096) ;
      __ma_debug_start = . ;
      *( .ma.debug.var*)
      __ma_debug_end = . ;
      . = ALIGN(4096) ;
      *( SORT(.ma.debug.size*) )
    }
  .text :
    {
      /* On regroupe les fonctions rarement utilisées */
      *(.ma.initfunc*)
    }
}
