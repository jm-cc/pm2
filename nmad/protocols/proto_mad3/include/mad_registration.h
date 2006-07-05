
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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
  mad_NMAD_TCP,
#endif // DRV_TCP

#ifdef DRV_SCTP
  mad_NMAD_SCTP,
#endif // DRV_SCTP

#ifdef DRV_GM
  mad_NMAD_GM,
#endif // DRV_GM

#ifdef DRV_MX
  mad_NMAD_MX,
#endif // DRV_MX

#ifdef DRV_QUADRICS
  mad_NMAD_QUADRICS,
#endif // DRV_QUADRICS

  mad_driver_number // Must be the last element of the enum declaration
} mad_driver_id_t, *p_mad_driver_id_t;

#define mad_DRIVER_DEFAULT 0

#endif /* MAD_REGISTRATION_H */
