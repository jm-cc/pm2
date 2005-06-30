
#section macros

//#ifdef __USE_UNIX98
# define MARCEL_RWLOCK_INITIALIZER \
  { .__rw_lock=MA_SPIN_LOCK_UNLOCKED, .__rw_readers=0, .__rw_writer=NULL,     \
    .__rw_read_waiting=NULL, .__rw_write_waiting=NULL,			      \
    .__rw_kind=MARCEL_RWLOCK_DEFAULT_NP, .__rw_pshared=MARCEL_PROCESS_PRIVATE }
//#endif
//#ifdef __USE_GNU
# define MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { .__rw_lock=MA_SPIN_LOCK_UNLOCKED, .__rw_readers=0, .__rw_writer=NULL,     \
    .__rw_read_waiting=NULL, .__rw_write_waiting=NULL,			      \
    .__rw_kind=MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, 		      \
    .__rw_pshared=MARCEL_PROCESS_PRIVATE }
//#endif

//#ifdef __USE_UNIX98
enum
{
  MARCEL_RWLOCK_PREFER_READER_NP,
  MARCEL_RWLOCK_PREFER_WRITER_NP,
  MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
  MARCEL_RWLOCK_DEFAULT_NP = MARCEL_RWLOCK_PREFER_WRITER_NP
};
//#endif	/* Unix98 */

#section structures
#depend "marcel_threads.h[types]"

/* Read-write locks.  */
typedef struct _marcel_rwlock_t
{
  struct _marcel_fastlock __rw_lock; /* Lock to guarantee mutual exclusion */
  int __rw_readers;                   /* Number of readers */
  p_marcel_task_t __rw_writer;         /* Identity of writer, or NULL if none */
  p_marcel_task_t __rw_read_waiting;   /* Threads waiting for reading */
  p_marcel_task_t __rw_write_waiting;  /* Threads waiting for writing */
  int __rw_kind;                      /* Reader/Writer preference selection */
  int __rw_pshared;                   /* Shared between processes or not */
} marcel_rwlock_t;


/* Attribute for read-write locks.  */
typedef struct
{
  int __lockkind;
  int __pshared;
} marcel_rwlockattr_t;

#section functions
#depend "marcel_alias.h[macros]"

DEC_MARCEL_UNIX98(int,
		 rwlock_init, (marcel_rwlock_t *rwlock,
			       const marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlock_destroy, (marcel_rwlock_t *rwlock) __THROW)

DEC_MARCEL_UNIX98(int,
		 rwlock_rdlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_XOPEN2K(int,
		 rwlock_timedrdlock, (marcel_rwlock_t *rwlock,
				      const struct timespec *abstime) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlock_tryrdlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlock_wrlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlock_trywrlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlock_unlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_init, (marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_destroy, (marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_getpshared,
		 (const marcel_rwlockattr_t *attr, int *pshared) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t *attr, int *pref) __THROW)
DEC_MARCEL_UNIX98(int,
		 rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref) __THROW)
