
/*
 * Mad_registration.h
 * ==================
 */

#ifndef MAD_REGISTRATION_H
#define MAD_REGISTRATION_H
/*
 * Protocol module identifier
 * --------------------------
 */
typedef enum
{
#ifdef DRV_TCP
  mad_TCP,
#endif /* DRV_TCP */
#ifdef DRV_VIA
  mad_VIA,
#endif /* DRV_VIA */
#ifdef DRV_SISCI
  mad_SISCI,
#endif /* DRV_SISCI */
#ifdef DRV_SBP
  mad_SBP,
#endif /* DRV_SBP */,
#ifdef DRV_MPI
  mad_MPI,
#endif /* DRV_MPI */,
#ifdef DRV_BIP
  mad_BIP,
#endif /* DRV_BIP */,
  mad_driver_number /* Must be the last element of the enum declaration */
} mad_driver_id_t, *p_mad_driver_id_t;

#define mad_DRIVER_DEFAULT 0

#endif /* MAD_REGISTRATION_H */
