#ifdef __USE_UNIX98

DEC_MARCEL_POSIX(int,
		 rwlock_init, (marcel_rwlock_t *rwlock,
			       const marcel_rwlockattr_t *attr))
DEC_MARCEL_POSIX(int,
		 rwlock_destroy, (marcel_rwlock_t *rwlock))

DEC_MARCEL_POSIX(int,
		 rwlock_rdlock, (marcel_rwlock_t *rwlock))
DEC_MARCEL_POSIX(int,
		 rwlock_timedrdlock, (marcel_rwlock_t *rwlock,
				      const struct timespec *abstime))
DEC_MARCEL_POSIX(int,
		 rwlock_tryrdlock, (marcel_rwlock_t *rwlock))
DEC_MARCEL_POSIX(int,
		 rwlock_wrlock, (marcel_rwlock_t *rwlock))
DEC_MARCEL_POSIX(int,
		 rwlock_trywrlock, (marcel_rwlock_t *rwlock))
DEC_MARCEL_POSIX(int,
		 rwlock_unlock, (marcel_rwlock_t *rwlock))
DEC_MARCEL_POSIX(int,
		 rwlockattr_init, (marcel_rwlockattr_t *attr))
DEC_MARCEL_POSIX(int,
		 rwlockattr_destroy, (marcel_rwlockattr_t *attr))
DEC_MARCEL_POSIX(int,
		 rwlockattr_getpshared,
		 (const marcel_rwlockattr_t *attr, int *pshared))
DEC_MARCEL_POSIX(int,
		 rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared))
DEC_MARCEL_POSIX(int,
		 rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t *attr, int *pref))
DEC_MARCEL_POSIX(int,
		 rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref))
#endif
