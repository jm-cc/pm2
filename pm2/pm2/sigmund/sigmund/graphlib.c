#include "graphlib.h"
#include "stdlib.h"
#include "assert.h"

static thread_on_list thr_on;
static lwp_on_list lwp_on;


void set_lwp_last_up(int lwp, u_64 last_up)
{
  lwp_on_list tmp;
  tmp = lwp_on;
  while (tmp != NULL) {
    if (tmp->lwp == lwp) {  tmp->last_up = last_up; return;}
    tmp = tmp->next;
  }
  tmp = (lwp_on_list) malloc(sizeof(struct lwp_on_list_st));
  assert(tmp != NULL);
  tmp->next = lwp_on;
  //  printf("Last Up %d %u\n", lwp, (unsigned) last_up);
  tmp->last_up = last_up;
  tmp->lwp = lwp;
  lwp_on = tmp;
}

u_64 get_lwp_last_up(int lwp)
{
  lwp_on_list tmp;
  tmp = lwp_on;
  //  printf("Get Last up %d\n", lwp);
  while (tmp != NULL) {
    if (tmp->lwp == lwp) return tmp->last_up;
    tmp = tmp->next;
  }
  return -1; // Erreur
}

void set_thread_disactived(int thread, int disactivated)
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) {  tmp->disactivated = disactivated; return;}
    tmp = tmp->next;
  }
  tmp = (thread_on_list) malloc(sizeof(struct thread_on_list_st));
  assert(tmp != NULL);
  tmp->next = thr_on;
  tmp->disactivated = disactivated;
  tmp->thread = thread;
  thr_on = tmp;
}

int get_thread_disactivated(int thread)
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) return tmp->disactivated;
    tmp = tmp->next;
  }
  return -1; // Erreur
}

