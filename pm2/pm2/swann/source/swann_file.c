/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: swann_file.c,v $
Revision 1.2  2000/03/27 12:54:06  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.1  2000/02/17 09:29:36  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

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
  while((status = access(pathname, amode)) == -1)
    {
      if (status == EACCES)
	{
	  LOG_OUT();
	  return tbx_false;
	}
      else if (status != EINTR)
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

  file->pathname = malloc(strlen(pathname) + 1);
  CTRL_ALLOC(file->pathname);

  strcpy(file->pathname, pathname);

  file->descriptor = -1;
  file->mode       = swann_file_mode_uninitialized;
  
  if ((file->exist = swann_file_access(pathname, F_OK)))
    {
      file->readable   = swann_file_access(pathname, R_OK);
      file->writeable  = swann_file_access(pathname, W_OK);
      file->executable = swann_file_access(pathname, X_OK);      
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
 * Read/Write  (block)
 * -------------------
 */
size_t
swann_file_read_block(p_swann_file_t  file,
		      void           *ptr,
		      size_t          length)
{
  int status;

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
  int status;

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




