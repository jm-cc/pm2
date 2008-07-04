
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

#section functions
#section marcel_macros [no-depend-previous]
#section marcel_types
#section marcel_structures
#section marcel_functions

#section functions
void marcel_enable_deviation(void);
void marcel_disable_deviation(void);

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg);
void marcel_do_deviate(marcel_t pid, handler_func_t h, any_t arg);

#section marcel_functions

void ma_deviate_init(void);
void marcel_execute_deviate_work(void);

#section marcel_types
typedef struct deviate_record_struct_t deviate_record_t;
typedef struct marcel_work marcel_work_t;

#section marcel_structures
struct deviate_record_struct_t {
  handler_func_t func;
  any_t arg;
  struct deviate_record_struct_t *next;
};

struct marcel_work {
  volatile unsigned has_work;
  deviate_record_t *deviate_work;  
};

#section marcel_macros
#define MARCEL_WORK_INIT ((marcel_work_t) { \
        .deviate_work=NULL \
})


/* flags indiquant le type de travail à faire */
enum {
  MARCEL_WORK_DEVIATE = 0x1,
};

#define HAS_WORK(pid)          ((pid)->has_work)

#define HAS_DEVIATE_WORK(pid) \
  ((pid)->work.has_work & MARCEL_WORK_DEVIATE)
#define SET_DEVIATE_WORK(pid) \
  do { (pid)->work.has_work |= MARCEL_WORK_DEVIATE; } while(0)
#define CLR_DEVIATE_WORK(pid) \
  do { (pid)->work.has_work &= ~MARCEL_WORK_DEVIATE; } while(0)

void ma_do_work(marcel_t self);

