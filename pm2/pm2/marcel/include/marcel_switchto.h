#section marcel_macros

/* effectue un setjmp. On doit �tre RUNNING avant et apr�s
 * */
#define MA_THR_SETJMP(current) \
  marcel_ctx_setjmp(current->ctx_yield)

/* On vient de reprendre la main.
 * */
#define MA_THR_RESTARTED(current, info) \
  do {                                 \
    MA_ARCH_SWITCHTO_LWP_FIX(current); \
    MA_ACT_SET_THREAD(current); \
    MTRACE(info, current);             \
  } while(0)

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * �tre RUNNING. La variable previous_task doit �tre correctement
 * positionn�e pour pouvoir annuler la propri�t� RUNNING du thread
 * courant.
 * */
#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    PROF_SWITCH_TO(cur_num, next->number); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)


#section marcel_functions

inline static marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next);

#section marcel_variables
MA_DECLARE_PER_LWP(marcel_task_t *, previous_thread);

#section marcel_inline

inline static marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next)
{
	MA_BUG_ON(!ma_in_atomic());
	if (cur != next) {
		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_THR_RESTARTED(cur, "Preemption");
			MA_BUG_ON(!ma_in_atomic());
			return __ma_get_lwp_var(previous_thread);
		}
		debug_printf(&MA_DEBUG_VAR_NAME(default),
			     "switchto(%p, %p) on LWP(%d)\n",
		       cur, next, LWP_NUMBER(GET_LWP(cur)));
		__ma_get_lwp_var(previous_thread)=cur;
		MA_THR_LONGJMP(cur->number, (next), NORMAL_RETURN);
	}
	return cur;
}

