SECTIONS {
/*  .rodata : 
    {
      . = ALIGN(8) ;
      ___ma_per_lwp_start = . ;
      *( SORT(.ma.per.lwp.offset.*))
      *(.ma.per.lwp.offset)
      ___ma_per_lwp_end = . ;
      ___ma_per_lwp_size = ___ma_per_lwp_end - ___ma_per_lwp_start ;
    }
*/
  .data : 
    {
      . = ALIGN(4096) ;
      *(.ma.runqueues)
      . = ALIGN(4096) ;
      ___ma_main_lwp_start = . ;
      *(.ma.main.lwp)
      *( SORT(.ma.per.lwp.main.*))
      *(.ma.per.lwp.main)
      ___ma_main_lwp_end = . ;
   /* ne marche pas, vu que .data n'est pas à l'adresse 0 */
   /* ___ma_main_lwp_size = ___ma_main_lwp_end - ___ma_main_lwp_start ; */

      . = ALIGN(4096) ;
      ___ma_debug_start = . ;
      *( .ma.debug.var*)
      ___ma_debug_end = . ;
      . = ALIGN(4096) ;
      *( SORT(.ma.debug.size*) )
      . = ALIGN(4096);
      ___ma_init_start = . ;
      *( SORT(.ma.init.*))
      ___ma_init_end = . ;
    }
  .text :
    {
      /* On regroupe les fonctions rarement utilisées */
      *(.ma.initfunc*)
    }
}
