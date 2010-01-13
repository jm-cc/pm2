#section functions

void
marcel_supervisor_init (void);

void
marcel_supervisor_sync (void);

void
marcel_supervisor_enable_nbour_vp (int vpnum);

void
marcel_supervisor_disable_nbour_vp (int vpnum);

void
marcel_supervisor_enable_nbour_vpset (const marcel_vpset_t * vpset);

void
marcel_supervisor_disable_nbour_vpset (const marcel_vpset_t * vpset);
