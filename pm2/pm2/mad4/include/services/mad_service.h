/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2003 "the PM2 team" (see AUTHORS file)
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
 * Mad_service.h
 * ==============================
 */

#ifndef MAD_SERVICE_H
#define MAD_SERVICE_H

#define SIZE_OFNAME           32
/* the total number of services except user*/
#define SERVICE_NUMBER         9

/* types de services */
typedef enum  {
   BUFFER_SERVICE  = 111,
   FACTORY_SERVICE = 222,
   FORWARD_SERVICE = 333,
   LINK_SERVICE    = 444,
   MAD3_SERVICE    = 555,
   MOTOR_SERVICE   = 666,
   NETWORK_SERVICE = 777,
   ROUTING_SERVICE = 888,  
   STATS_SERVICE   = 999,
   USER_SERVICE    = 696  
} mad_service_type_t;


typedef enum {
  mad_RECV  = 0,
  mad_SEND  = 1,
  NO_WAY    = -1   
} mad_way_t ;

/* Un service */
struct s_mad_service {
   
   /* type de service */
   mad_service_type_t type;
   
   /* Nom du service */
   char  name[SIZE_OFNAME];
   
   /* API du service */
   struct s_mad_service_interface *interface;
   
   /* Marqueur d'activité  : volatile ??*/
   int active;
   
   /* Table des attributs (nom de table, struct s_mad_attribute *) */
   p_tbx_htable_t  attribute_table;
   
   /* Emplacement pour donnees supplementaires du service */
   void       *specific;
};

typedef struct s_mad_service  mad_service_t ;
typedef struct s_mad_service *p_mad_service_t ;
typedef mad_service_type_t mad_service_array_t[SERVICE_NUMBER];
typedef char mad_service_names_t [SERVICE_NUMBER][SIZE_OFNAME];

extern mad_service_names_t service_names;
extern mad_service_array_t service_types;


#endif /* MAD_SERVICE_H */
