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
      __ma_main_lwp_start = . ;
      *(.ma.main.lwp)
      *( SORT(.ma.per.lwp.main.*))
      *(.ma.per.lwp.main)
      __ma_main_lwp_end = . ;
   /* ne marche pas, vu que .data n'est pas à l'adresse 0 */
   /* __ma_main_lwp_size = __ma_main_lwp_end - __ma_main_lwp_start ; */

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
  .rodata :
    {
      . = ALIGN(8);
      __ma_init_start = . ;
      *( SORT(.ma.init.*))
      __ma_init_end = . ;
    }
}
