#ifndef PAPI_MARCEL_H
#define PAPI_MARCEL_H

struct s_papimar_event_set;

struct s_papimar_context;

void
papimar_error(const char *msg, const int code);

void
papimar_init_event_set(struct s_papimar_event_set **p_papimar_event_set);

void
papimar_add_event(struct s_papimar_event_set *papimar_event_set, int event_num);

void
papimar_start(struct s_papimar_event_set *papimar_event_set);

void
papimar_stop(struct s_papimar_event_set *papimar_event_set);

void
papimar_init_context(struct s_papimar_event_set *papimar_event_set, struct s_papimar_context **p_papimar_context);

void
papimar_suspend_context(struct s_papimar_context * const papimar_context);

void
papimar_resume_context(struct s_papimar_context * const papimar_context);

void
papimar_read_context(struct s_papimar_context * const papimar_context, long long * const counters);

void
papimar_accumulate_context(struct s_papimar_context * const papimar_context, long long * const counters);

void
papimar_reset_context(struct s_papimar_context * const papimar_context);

#endif

