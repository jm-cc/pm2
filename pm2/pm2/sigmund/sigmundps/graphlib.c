#include "graphlib.h"
#include "stdlib.h"
#include "assert.h"

static thread_on_list thr_on;
static pid_on_list pid_on;

/* This file is used to deal with the graphic library */
void set_pid_last_up(int pid, u_64 last_up)
{
  pid_on_list tmp;
  tmp = pid_on;
  while (tmp != NULL) {
    if (tmp->pid == pid) {  tmp->last_up = last_up; return;}
    tmp = tmp->next;
  }
  tmp = (pid_on_list) malloc(sizeof(struct pid_on_list_st));
  assert(tmp != NULL);
  tmp->next = pid_on;
  tmp->last_up = last_up;
  tmp->pid = pid;
  pid_on = tmp;
}

u_64 get_pid_last_up(int pid)
{
  pid_on_list tmp;
  tmp = pid_on;
  while (tmp != NULL) {
    if (tmp->pid == pid) return tmp->last_up;
    tmp = tmp->next;
  }
  return -1; // Error
}

void set_thread_disactivated(int thread, int disactivated)
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
  tmp->dec = 0;
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
  return -1; // Error
}

mode get_last_type_thread(int thread)
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) return tmp->type;
    tmp = tmp->next;
  }
  return USER; // Error
}

void set_last_type_thread(int thread, mode type)
{
 thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) {tmp->type = type; return;}
    tmp = tmp->next;
  }
  return; // Error
}

/*
void set_thread_last_up(int thread, u_64 last_up)   // ?
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) {  tmp->last_up = last_up; return;}
    tmp = tmp->next;
  }
  tmp = (thread_on_list) malloc(sizeof(struct thread_on_list_st));
  assert(tmp != NULL);
  tmp->next = thr_on;
  tmp->last_up = last_up;
  tmp->disactivated = 1;
  tmp->thread = thread;
  tmp->dec = 0;
  thr_on = tmp;
}

u_64 get_thread_last_up(int thread)           // ?
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) return tmp->last_up;
    tmp = tmp->next;
  }
  return -1; // Error
}
*/

void add_thread_dec(int thread, int add)
{
  thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) {tmp->dec += add; return;}
    tmp = tmp->next;
  }
}

int get_thread_dec(int thread)
{
 thread_on_list tmp;
  tmp = thr_on;
  while (tmp != NULL) {
    if (tmp->thread == thread) return tmp->dec;
    tmp = tmp->next;
  }
  return -1; //Erreur
}
