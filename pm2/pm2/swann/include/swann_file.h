
/*
 * swann_file.h
 * ------------
 */

#ifndef __SWANN_FILE_H
#define __SWANN_FILE_H

typedef enum
{
  swann_file_state_uninitialized = 0,
  swann_file_state_initialized,
  swann_file_state_open,
  swann_file_state_closed,
} swann_file_state_t, *p_swann_file_state_t;

typedef enum
{
  swann_file_mode_uninitialized,
  swann_file_mode_read,
  swann_file_mode_write,
  swann_file_mode_exec,
} swann_file_mode_t, *p_swann_file_mode_t;

typedef struct s_swann_file
{
  char               *pathname;
  int                 descriptor;
  swann_file_state_t  state;
  swann_file_mode_t   mode;
  tbx_bool_t          exist;
  tbx_bool_t          readable;
  tbx_bool_t          writeable;
  tbx_bool_t          executable;
} swann_file_t;


#endif /* __SWANN_FILE_H */
