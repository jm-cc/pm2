

#include "clone.h"
#include "sys/archdep.h"

marcel_key_t _clone_key, _slave_key;

void clone_init(clone_t *c, int nb_slaves)
{
  marcel_key_create(&_clone_key, NULL);
  marcel_key_create(&_slave_key, NULL);

  marcel_mutex_init(&c->mutex, NULL);
  marcel_cond_init(&c->master, NULL);
  marcel_cond_init(&c->slave, NULL);
  c->total = nb_slaves + 1;
  c->nb = c->nbs = c->terminate = 0;
}

void clone_slave(clone_t *c)
{
  jmp_buf buf;

  marcel_mutex_lock(&c->mutex);

  if(++c->nb != c->total) {
    marcel_cond_wait(&c->slave, &c->mutex);
  } else {
    marcel_cond_broadcast(&c->slave);
    c->nb = 0;
  }

  if(c->terminate) {
    marcel_mutex_unlock(&c->mutex);
    marcel_exit(NULL);
  }

  marcel_setspecific(_clone_key,
		     (any_t)((long)marcel_stackbase(marcel_self()) -
			     (long)marcel_stackbase(c->master_pid)));

  memcpy(&buf, &c->master_jb, sizeof(jmp_buf));

  (long)SP_FIELD(buf) = (long)SP_FIELD(buf) + clone_my_delta();
#ifdef FP_FIELD
  (long)FP_FIELD(buf) = (long)FP_FIELD(buf) + clone_my_delta();
#endif

  marcel_mutex_unlock(&c->mutex);

  longjmp(buf, 1);
}

void clone_slave_ends(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(++c->nbs == c->total - 1) {
    marcel_cond_signal(&c->master);
  }

  marcel_mutex_unlock(&c->mutex);

  longjmp(my_data()->buf, 1);
}

void clone_master(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(++c->nb != c->total)
    marcel_cond_wait(&c->slave, &c->mutex);
  else {
    marcel_cond_broadcast(&c->slave);
    c->nb = 0;
  }

  marcel_mutex_unlock(&c->mutex);
}

void clone_master_ends(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(c->nbs != c->total - 1)
    marcel_cond_wait(&c->master, &c->mutex);
  c->nbs = 0;

  marcel_mutex_unlock(&c->mutex);
}

void clone_terminate(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  c->terminate = 1;
  marcel_cond_broadcast(&c->slave);

  marcel_mutex_unlock(&c->mutex);
}
