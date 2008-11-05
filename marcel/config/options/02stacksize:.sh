# Set the thread stack size, in KiB.

stack_size_in_bytes="$(expr $1 \* 1024)"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_THREAD_SLOT_SIZE=$stack_size_in_bytes"
