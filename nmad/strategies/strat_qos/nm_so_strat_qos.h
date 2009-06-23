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

#ifdef NMAD_QOS

/* Threshold used on the sending side to decide if data should be
   copied (when smaller than the threshold) within the packet wrapper
   header zone */
#define NM_SO_COPY_ON_SEND_THRESHOLD        (32 * 1024)

extern int nm_so_strat_qos_init(void);

/* Parameters for the QoS strategy
 */

enum nm_qos_policy_e
  {
    NM_SO_POLICY_FIFO             = 0,
    NM_SO_POLICY_LATENCY          = 1,
    NM_SO_POLICY_RATE             = 2,
    NM_SO_POLICY_PRIORITY         = 3,
    NM_SO_POLICY_PRIORITY_LATENCY = 4,
    NM_SO_POLICY_PRIORITY_RATE    = 5,
    NM_SO_POLICY_NONE             = 6
  };
typedef enum nm_qos_policy_e nm_qos_policy_t;

#define NM_SO_POLICY_UNDEFINED NM_SO_POLICY_NONE
#define NM_SO_NB_POLICIES      NM_SO_POLICY_NONE
#define NM_SO_NB_PRIORITIES 3


/** Set the specified priority value to the specified tag
 *  @param tag the tag of a flow
 *  @param priority the priority value to map to the tag
 */
extern void nm_so_set_priority(nm_tag_t tag, uint8_t priority);

/** Get the priority value for the specified tag
 *  @param tag the tag of a flow
 *  @return The priority value mapped to the tag
 */
extern uint8_t nm_so_get_priority(nm_tag_t tag);

/** Set the specified policy to the specified tag
 *  @param tag the tag of a flow
 *  @param policy the policy value to map to the tag
 */
extern void nm_so_set_policy(nm_tag_t tag, nm_qos_policy_t policy);

/** Get the policy for the specified tag
 *  @param tag the tag of a flow
 *  @return The policy mapped to the tag
 */
extern nm_qos_policy_t nm_so_get_policy(nm_tag_t tag);

#endif /* NMAD_QOS */
