
/*
 * swann_file.c
 * ============
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "swann.h"

/*
 * Static functions
 * ================
 */

/*
 * Syscall Wrappers
 * ----------------
 */

tbx_bool_t
swann_file_access(char *pathname,
		  int   amode)
{
  int status;
  
  LOG_IN();
  DISP_STR("swann_file_access: file", pathname);
  
  while((status = access(pathname, amode)) == -1)
    {
      if (errno == EACCES)
	{
	  LOG_OUT();
	  return tbx_false;
	}
      else if (errno != EINTR)
	{
	  perror("access");
	  FAILURE("swann_file_access");
	}
    }

  if (status == 0)
    {
      LOG_OUT();
      return tbx_true;
    }
  else
    FAILURE("invalid `access' syscall return value");
}

/*
 * Public functions
 * ================
 */

/*
 * Initialization
 * --------------
 */
swann_status_t
swann_file_init(p_swann_file_t  file,
		char           *pathname)
{
  LOG_IN();
  if (file->state != swann_file_state_uninitialized)
    FAILURE("file already initialized");

  if (pathname[0] == '~')
    {
      file->pathname = malloc(strlen(pathname) + strlen(getenv("HOME")));
      CTRL_ALLOC(file->pathname);

      strcpy(file->pathname, getenv("HOME"));
      strcat(file->pathname, pathname + 1);
    }
  else
    {      
      file->pathname = malloc(strlen(pathname) + 1);
      CTRL_ALLOC(file->pathname);

      strcpy(file->pathname, pathname);
    }
  
  file->descriptor = -1;
  file->mode       = swann_file_mode_uninitialized;
  
  if ((file->exist = swann_file_access(file->pathname, F_OK)))
    {
      file->readable   = swann_file_access(file->pathname, R_OK);
      file->writeable  = swann_file_access(file->pathname, W_OK);
      file->executable = swann_file_access(file->pathname, X_OK);      
    }
  else
    {
      file->readable   = tbx_false;
      file->writeable  = tbx_false;
      file->executable = tbx_false;
    }
  
  file->state = swann_file_state_initialized;
  LOG_OUT();

  return swann_success;
}


/*
 * Open
 * ----
 */
swann_status_t
swann_file_open(p_swann_file_t file)
{
  int status;
  
  LOG_IN();
  if (   file->state != swann_file_state_initialized
      && file->state != swann_file_state_closed)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_uninitialized)
    FAILURE("invalid file mode");

  if (!file->exist)
    FAILURE("file not found");

  if (!file->readable)
    FAILURE("file not readable");

  while ((status = open(file->pathname,
			O_RDONLY)) == -1)
    if (errno != EINTR)
      {
	perror("open");
	FAILURE("swann_file_open");
      }
  
  file->descriptor = status;
  file->state      = swann_file_state_open;
  file->mode       = swann_file_mode_read;
  LOG_OUT();

  return swann_success;
}

swann_status_t
swann_file_create(p_swann_file_t file)
{
  int status;
  
  LOG_IN();
  if (   file->state != swann_file_state_initialized
      && file->state != swann_file_state_closed)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_uninitialized)
    FAILURE("invalid file mode");

  if (!file->writeable)
    FAILURE("file not writeable");

  while((status = creat(file->pathname,
		       S_IRUSR|S_IWUSR)) == -1)
    if (errno != EINTR)
      {
	perror("creat");
	FAILURE("swann_file_create");
      }
  
  file->descriptor = status;
  file->state      = swann_file_state_open;
  file->mode       = swann_file_mode_write;
  file->readable   = tbx_true;
  file->writeable  = tbx_true;
  file->executable = tbx_false;
  
  LOG_OUT();

  return swann_success;
}


/*
 * Close
 * -----
 */
swann_status_t
swann_file_close(p_swann_file_t file)
{
  int status;

  LOG_IN();
  if (file->state != swann_file_state_open)
    FAILURE("invalid file state");

  if (   file->mode != swann_file_mode_read
      && file->mode != swann_file_mode_write)
    FAILURE("invalid file mode");

  if (!file->exist)
    FAILURE("file not found");

  while ((status = close(file->descriptor)) == -1)
    if (errno != EINTR)
      {
	perror("close");
	FAILURE("swann_file_close");
      }

  file->descriptor = -1;
  file->state      = swann_file_state_closed;
  file->mode       = swann_file_mode_uninitialized;
  
  LOG_OUT();

  return swann_success;
}


/*
 * Destroy
 * -------
 */
swann_status_t
swann_file_destroy(p_swann_file_t file)
{
  LOG_IN();
  if (   file->state != swann_file_state_uninitialized
      && file->state != swann_file_state_initialized
      && file->state != swann_file_state_closed)
    FAILURE("invalid file state");

  if (file->state != swann_file_state_uninitialized)
    {
      free(file->pathname);
    }
  
  file->state      = swann_file_state_uninitialized;
  file->pathname   = NULL;
  file->descriptor = -1;
  file->mode       = swann_file_mode_uninitialized;
  file->exist      = tbx_false;
  file->readable   = tbx_false;
  file->writeable  = tbx_false;
  file->executable = tbx_false;
  LOG_OUT();

  return swann_success;
}


/*
 * Read/Write (block level)
 * ----------...............
 */
size_t
swann_file_read_block(p_swann_file_t  file,
		      void           *ptr,
		      size_t          length)
{
  size_t status;

  LOG_IN();
  if (file->state != swann_file_state_open)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_read)
    FAILURE("invalid file mode");

  while ((status = read(file->descriptor,
			ptr,
			length)) == -1)
    if (errno != EINTR)
      {
	perror("read");
	FAILURE("swann_file_read_block");
      }

  LOG_OUT();

  return status;
}

size_t
swann_file_write_block(p_swann_file_t file,
		      void           *ptr,
		      size_t          length)
{
  size_t status;

  LOG_IN();
  if (file->state != swann_file_state_open)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_write)
    FAILURE("invalid file mode");

  while ((status = write(file->descriptor,
			ptr,
			length)) == -1)
    if (errno != EINTR)
      {
	perror("write");
	FAILURE("swann_file_write_block");
      }

  LOG_OUT();

  return status;
}

/*
 * File transfer (block level)
 * -------------...............
 */
swann_status_t
swann_file_send_block(p_swann_net_client_t client,
		      p_swann_file_t       file)
{
  char               buffer[TBX_FILE_TRANSFER_BLOCK_SIZE];
  ntbx_pack_buffer_t pack_buffer;
  size_t             block_size;
  
  LOG_IN();
  block_size =
    swann_file_read_block(file, buffer, TBX_FILE_TRANSFER_BLOCK_SIZE);
  
  ntbx_pack_int((int)block_size, &pack_buffer);

  while (!ntbx_tcp_write_poll(1, &client->client));
  ntbx_tcp_write_block(client->client,
		       &pack_buffer,
		       sizeof(ntbx_pack_buffer_t));

  if (block_size)
    {
      while (!ntbx_tcp_write_poll(1, &client->client));
      ntbx_tcp_write_block(client->client, buffer, block_size);

      LOG_OUT();
      return swann_success;
    }
  else
    {
      LOG_OUT();
      return swann_failure;
    }
}

swann_status_t
swann_file_receive_block(p_swann_net_client_t client,
			 p_swann_file_t       file)
{
  char               buffer[TBX_FILE_TRANSFER_BLOCK_SIZE];
  ntbx_pack_buffer_t pack_buffer;
  size_t             block_size;

  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client->client));
  ntbx_tcp_read_block(client->client,
		      &pack_buffer,
		      sizeof(ntbx_pack_buffer_t));
  block_size = (int)ntbx_unpack_int(&pack_buffer);

  if (block_size)
    {
      while (!ntbx_tcp_write_poll(1, &client->client));
      ntbx_tcp_write_block(client->client, buffer, block_size);
      LOG_OUT();
      return swann_success;
    }
  else
    {
      LOG_OUT();
      return swann_failure;
    }
}

