
#include "pm2.h"
#include "sys/pm2_printf.h"

#include <stdarg.h>

static unsigned PM2_PRINTF;

static marcel_sem_t print_mutex;
static char _pm2_print_buf[1024] __MAD_ALIGNED__;

extern int pm2_main_module(void);

void pm2_printf(char *format, ...)
{
  va_list args;
  char *ptr;

  if(pm2_main_module() == __pm2_self) {
    lock_task();
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    unlock_task();
  } else {
    marcel_sem_P(&print_mutex);
    sprintf(_pm2_print_buf, "[t%x] ", __pm2_self);
    ptr = _pm2_print_buf + strlen(_pm2_print_buf);
    va_start(args, format);
    vsprintf(ptr, format, args);
    va_end(args);
    {
      unsigned len = strlen(_pm2_print_buf)+1;

      pm2_rawrpc_begin(pm2_main_module(), PM2_PRINTF, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, _pm2_print_buf, len);
      pm2_rawrpc_end();
    }
    marcel_sem_V(&print_mutex);
  }
}

static void netserver_printf(void)
{ 
  static char buf[2048] __MAD_ALIGNED__;
  unsigned len;

  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, buf, len);

  pm2_rawrpc_waitdata();

  tfprintf(stderr, "%s", buf);
}

void pm2_printf_init(void)
{
  marcel_sem_init(&print_mutex, 1);

  pm2_rawrpc_register(&PM2_PRINTF, netserver_printf);
}

