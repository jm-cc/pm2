SECTIONS {
  .data : 
    {
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
