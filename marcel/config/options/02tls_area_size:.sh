# Set the TLS area size, in KiB.

tls_area_size_in_bytes="$(expr $1 \* 1024)"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMA_TLS_AREA_SIZE=$tls_area_size_in_bytes"
