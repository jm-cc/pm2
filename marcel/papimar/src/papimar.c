#include <marcel.h>
#include <papi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <papimar_config.h>

#include "papimar.h"

struct s_papimar_event_set {
	int *EventSet;
	int nb_counters;
};


struct s_papimar_context {
	struct s_papimar_event_set *event_set;
	long long *sum;
	long long *start;
};

/* --- */

static
void
__papimar_read_context(const struct s_papimar_context * const papimar_context, long long * const counters);

static
void
__papimar_accumulate_context(struct s_papimar_context * const papimar_context, long long * const counters);

static
void
__papimar_reset_context(struct s_papimar_context * const papimar_context);

/* --- */

void
papimar_error(const char *msg, const int code) {
		tfprintf(stderr, "PAPI error: %s, error code %d\n", msg, code);
		exit(1);
}

static void *
f_papimar_init_event_set(void *_arg) {
	struct s_papimar_event_set *evset = _arg;
	int err;

	marcel_extlib_protect();
	if ((err = PAPI_create_eventset(&(evset->EventSet[marcel_current_vp()]))) != PAPI_OK)
		papimar_error("PAPI_create_eventset", err);
	marcel_extlib_unprotect();

	return NULL;
}

void
papimar_init_event_set(struct s_papimar_event_set **p_evset) {
	struct s_papimar_event_set *evset;
	int nbvps = marcel_nbvps();
	marcel_attr_t attr;
	int i;

	evset = tmalloc(sizeof(*evset));
	evset->nb_counters = 0;
	evset->EventSet = tmalloc(nbvps * sizeof(int));

	marcel_attr_init(&attr);
	for (i = 0; i < nbvps; i++) {
		marcel_t t;
		evset->EventSet[i] = PAPI_NULL;
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i));
		marcel_create(&t, &attr, f_papimar_init_event_set, evset);
		marcel_join(t, NULL);
	}

	*p_evset = evset;
}

struct __s_papimar_add_event {
	struct s_papimar_event_set *evset;
	int event_num;
};

static void *
f_papimar_add_event(void *_arg) {
	struct __s_papimar_add_event *arg = _arg;
	int err;

	marcel_extlib_protect();
	if ((err = PAPI_add_event(arg->evset->EventSet[marcel_current_vp()], arg->event_num)) != PAPI_OK) {
		papimar_error("PAPI_add_event", err);
	}
	marcel_extlib_unprotect();

	return NULL;
}
void
papimar_add_event(struct s_papimar_event_set *evset, int event_num) {
	int nbvps = marcel_nbvps();
	struct __s_papimar_add_event arg;
	marcel_attr_t attr;
	int i;

	arg.evset = evset;
	arg.event_num = event_num;

	marcel_attr_init(&attr);
	for (i = 0; i < nbvps; i++) {
		marcel_t t;
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i));
		marcel_create(&t, &attr, f_papimar_add_event, &arg);
		marcel_join(t, NULL);
	}
	evset->nb_counters++;
}

static void *
f_papimar_start(void *_arg) {
	struct s_papimar_event_set *evset = _arg;
	int err;

	marcel_extlib_protect();
	if ((err = PAPI_start(evset->EventSet[marcel_current_vp()])) != PAPI_OK)
		papimar_error("PAPI_start", err);
	marcel_extlib_unprotect();

	return NULL;
}

void
papimar_start(struct s_papimar_event_set *evset) {
	int nbvps = marcel_nbvps();
	marcel_attr_t attr;
	int i;

	marcel_attr_init(&attr);
	for (i = 0; i < nbvps; i++) {
		marcel_t t;
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i));
		marcel_create(&t, &attr, f_papimar_start, evset);
		marcel_join(t, NULL);
	}

}

static void *
f_papimar_stop(void *_arg) {
	struct s_papimar_event_set *evset = _arg;
	long long counters[evset->nb_counters];
	int err;

	marcel_extlib_protect();
	if ((err = PAPI_stop(evset->EventSet[marcel_current_vp()], counters)) != PAPI_OK)
		papimar_error("PAPI_stop", err);
	marcel_extlib_unprotect();

	return NULL;
}

void
papimar_stop(struct s_papimar_event_set *evset) {
	int nbvps = marcel_nbvps();
	marcel_attr_t attr;
	int i;

	marcel_attr_init(&attr);
	for (i = 0; i < nbvps; i++) {
		marcel_t t;
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i));
		marcel_create(&t, &attr, f_papimar_stop, evset);
		marcel_join(t, NULL);
	}
}


void
papimar_init_context(struct s_papimar_event_set *evset, struct s_papimar_context **p_ctxt) {
	struct s_papimar_context *ctxt;
	int i;

	ctxt = tmalloc(sizeof(*ctxt));
	ctxt->event_set = evset;

	ctxt->sum = tmalloc(evset->nb_counters * sizeof (long long));
	ctxt->start = tmalloc(evset->nb_counters * sizeof (long long));

	for (i = 0; i < evset->nb_counters; i++) {
		ctxt->sum[i] = 0;
	}

	*p_ctxt = ctxt;
}

static
void
__papimar_read_context(const struct s_papimar_context * const ctxt, long long * const counters) {
	const int nb_counters = ctxt->event_set->nb_counters;
	long long * const sum	= ctxt->sum;
	int i;

	for (i = 0; i < nb_counters; i++) {
		counters[i] = sum[i];
	}
}

static
void
__papimar_accumulate_context(struct s_papimar_context * const ctxt, long long * const counters) {
	const int nb_counters = ctxt->event_set->nb_counters;
	long long * const sum	= ctxt->sum;
	int i;

	for (i = 0; i < nb_counters; i++) {
		counters[i] += sum[i];
		sum[i] = 0;
	}
}

static
void
__papimar_reset_context(struct s_papimar_context * const ctxt) {
	const int nb_counters	= ctxt->event_set->nb_counters;
	long long * const sum	= ctxt->sum;
	int i;

	for (i = 0; i < nb_counters; i++) {
		sum[i] = 0;
	}
}

void
papimar_suspend_context(struct s_papimar_context * const ctxt) {
	const int nb_counters	= ctxt->event_set->nb_counters;
	long long counters[nb_counters];
	int EventSet	= ctxt->event_set->EventSet[marcel_current_vp()];
	int err;

	if ((err = PAPI_read(EventSet, counters)) != PAPI_OK)
		papimar_error("PAPI_read", err);

	if ((err = PAPI_reset(EventSet)) != PAPI_OK)
		papimar_error("PAPI_reset", err);

	{
		long long * const sum	= ctxt->sum;
		long long * const start	= ctxt->start;
		int i;

		for (i = 0; i < nb_counters; i++) {
			sum[i] += counters[i] - start[i];
		}
	}
}

void
papimar_resume_context(struct s_papimar_context * const ctxt) {
	int EventSet	= ctxt->event_set->EventSet[marcel_current_vp()];
	int err;

	if ((err = PAPI_read(EventSet, ctxt->start)) != PAPI_OK)
		papimar_error("PAPI_read", err);
}

void
papimar_read_context(struct s_papimar_context * const ctxt, long long * const counters) {
	marcel_thread_preemption_disable();
	papimar_suspend_context(ctxt);
	__papimar_read_context(ctxt, counters);
	papimar_resume_context(ctxt);
	marcel_thread_preemption_enable();
}

void
papimar_accumulate_context(struct s_papimar_context * const ctxt, long long * const counters) {
	marcel_thread_preemption_disable();
	papimar_suspend_context(ctxt);
	__papimar_accumulate_context(ctxt, counters);
	papimar_resume_context(ctxt);
	marcel_thread_preemption_enable();
}

void
papimar_reset_context(struct s_papimar_context * const ctxt) {
	marcel_thread_preemption_disable();
	papimar_suspend_context(ctxt);
	__papimar_reset_context(ctxt);
	papimar_resume_context(ctxt);
	marcel_thread_preemption_enable();
}

