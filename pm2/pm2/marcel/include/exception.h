
#ifndef MARCEL_EXCEPTION_EST_DEF
#define MARCEL_EXCEPTION_EST_DEF

typedef char *exception;

extern exception
  TASKING_ERROR,
  DEADLOCK_ERROR,
  STORAGE_ERROR,
  CONSTRAINT_ERROR,
  PROGRAM_ERROR,
  STACK_ERROR,
  TIME_OUT,
  NOT_IMPLEMENTED,
  USE_ERROR,
  LOCK_TASK_ERROR;

#define BEGIN          { _exception_block _excep_blk; \
                       marcel_t __cur = marcel_self(); \
                       _excep_blk.old_blk=__cur->cur_excep_blk; \
                       __cur->cur_excep_blk=&_excep_blk; \
                       if (setjmp(_excep_blk.jb) == 0) {
#define EXCEPTION      __cur->cur_excep_blk=_excep_blk.old_blk; } \
                       else { __cur->cur_excep_blk=_excep_blk.old_blk; \
                       if(always_false) {
#define WHEN(ex)       } else if (__cur->cur_exception == ex) {
#define WHEN2(ex1,ex2) } else if (__cur->cur_exception == ex1 || \
                                  __cur->cur_exception == ex2) {
#define WHEN3(ex1,ex2,ex3) } else if (__cur->cur_exception == ex1 || \
                                      __cur->cur_exception == ex2 || \
                                      __cur->cur_exception == ex3) {
#define WHEN_OTHERS    } else if(!always_false) {
#define END            } else _raise(NULL); }}

#define RAISE(ex)      (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_raise(ex))
#define RRAISE         (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_raise(NULL))

/* =================== Quelques definitions utiles ==================== */

#define LOOP(name)	{ _exception_block _##name##_excep_blk; \
	int _##name##_val; \
	_##name##_excep_blk.old_blk=marcel_self()->cur_excep_blk; \
	marcel_self()->cur_excep_blk=&_##name##_excep_blk; \
	if ((_##name##_val = setjmp(_##name##_excep_blk.jb)) == 0) { \
	while(!always_false) {

#define EXIT_LOOP(name) longjmp(_##name##_excep_blk.jb, 2)

#define END_LOOP(name) } } \
	marcel_self()->cur_excep_blk=_##name##_excep_blk.old_blk; \
	if(_##name##_val == 1) _raise(NULL); }


_PRIVATE_ extern volatile boolean always_false;
_PRIVATE_ int _raise(exception ex);

#endif
