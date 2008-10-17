/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifndef NM_GATE_H
#define NM_GATE_H


struct nm_drv;
struct nm_core;
struct nm_sched;
struct nm_gate_drv;
struct nm_pkt_wrap;


/** Per driver gate related data. */
struct nm_gate_drv {

  /** Driver.
   */
  struct nm_drv           *p_drv;
  struct puk_receptacle_NewMad_Driver_s receptacle;
  puk_instance_t instance;
  
  /** Array of reference to current incoming requests, (indexed by trk_id).
   */
  struct nm_pkt_wrap	**p_in_rq_array;
  
  /** Cumulated number of pending out requests on this driver for
      this gate.
  */
  uint16_t		   out_req_nb;
};

/** Connexion to another process.
 */
struct nm_gate {

  /** NM core object. */
  struct nm_core		 *p_core;
  
  /** Gate id. */
  uint8_t			  id;
  
  /** SchedOpt fields. */
  struct nm_so_gate*p_so_gate;
  
  /** Gate data for each driver. */
  struct nm_gate_drv*p_gate_drv_array[NM_DRV_MAX];
 
};

#endif /* NM_GATE_H */
