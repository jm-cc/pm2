
#ifndef MARCEL_POLLING_EST_DEF
#define MARCEL_POLLING_EST_DEF


#define MAX_POLL_IDS    16

_PRIVATE_ struct _poll_struct;
_PRIVATE_ struct _poll_cell_struct;

typedef struct _poll_struct *marcel_pollid_t;
typedef struct _poll_cell_struct *marcel_pollinst_t;

typedef void (*marcel_pollgroup_func_t)(marcel_pollid_t id);

typedef void *(*marcel_poll_func_t)(marcel_pollid_t id,
				    unsigned active,
				    unsigned sleeping,
				    unsigned blocked);

typedef void *(*marcel_fastpoll_func_t)(marcel_pollid_t id,
					any_t arg,
					boolean first_call);

#define MARCEL_POLL_AT_TIMER_SIG  1
#define MARCEL_POLL_AT_YIELD      2
#define MARCEL_POLL_AT_LIB_ENTRY  4
#define MARCEL_POLL_AT_IDLE       8

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     marcel_fastpoll_func_t h,
				     unsigned polling_points);

#define MARCEL_POLL_FAILED                NULL
#define MARCEL_POLL_OK                    (void*)1
#define MARCEL_POLL_SUCCESS(id)           ((id)->cur_cell)
#define MARCEL_POLL_SUCCESS_FOR(pollinst) (pollinst)

void marcel_poll(marcel_pollid_t id, any_t arg);

#define FOREACH_POLL(id, _arg) \
  for((id)->cur_cell = (id)->first_cell; \
      (id)->cur_cell != NULL && (_arg = (typeof(_arg))((id)->cur_cell->arg)); \
      (id)->cur_cell = (id)->cur_cell->next)

#define GET_CURRENT_POLLINST(id) ((id)->cur_cell)

_PRIVATE_ typedef struct _poll_struct {
  unsigned polling_points;
  unsigned nb_cells;
  marcel_poll_func_t func;
  marcel_fastpoll_func_t fastfunc;
  struct _poll_cell_struct *first_cell, *cur_cell;
  struct _poll_struct *prev, *next;
  marcel_pollgroup_func_t gfunc;
  void *specific;
} poll_struct_t;

static __inline__ void marcel_pollid_setspecific(marcel_pollid_t id,
						 void *specific)
{
  id->specific = specific;
}

static __inline__ void *marcel_pollid_getspecific(marcel_pollid_t id)
{
  return id->specific;
}

// =============== PRIVATE ===============

_PRIVATE_ typedef struct _poll_cell_struct {
  marcel_t task;
  boolean blocked;
  any_t arg;
  struct _poll_cell_struct *prev, *next;
} poll_cell_t;

_PRIVATE_ extern poll_struct_t *__polling_tasks;
_PRIVATE_ int __marcel_check_polling(unsigned polling_point);

// TODO: use polling_point to evaluate more precisely if polling is
// necessary at this point.
_PRIVATE_ static __inline__ int marcel_polling_is_required(unsigned polling_point)
__attribute__ ((unused));
_PRIVATE_ static __inline__ int marcel_polling_is_required(unsigned polling_point)
{
  return __polling_tasks != NULL;
}
_PRIVATE_ static __inline__ int marcel_check_polling(unsigned polling_point)
__attribute__ ((unused));
_PRIVATE_ static __inline__ int marcel_check_polling(unsigned polling_point)
{
  if(marcel_polling_is_required(polling_point))
    return __marcel_check_polling(polling_point);
  return 0;
}

#endif
