#ifdef __USE_UNIX98

DEC_MARCEL_POSIX(int,
		 rwlock_init, (marcel_rwlock_t *rwlock,
			       const marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlock_destroy, (marcel_rwlock_t *rwlock) __THROW)

DEC_MARCEL_POSIX(int,
		 rwlock_rdlock, (marcel_rwlock_t *rwlock) __THROW)
#ifdef __USE_XOPEN2K
DEC_MARCEL_POSIX(int,
		 rwlock_timedrdlock, (marcel_rwlock_t *rwlock,
				      const struct timespec *abstime) __THROW)
#endif
DEC_MARCEL_POSIX(int,
		 rwlock_tryrdlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlock_wrlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlock_trywrlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlock_unlock, (marcel_rwlock_t *rwlock) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_init, (marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_destroy, (marcel_rwlockattr_t *attr) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_getpshared,
		 (const marcel_rwlockattr_t *attr, int *pshared) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t *attr, int *pref) __THROW)
DEC_MARCEL_POSIX(int,
		 rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref) __THROW)
#endif
