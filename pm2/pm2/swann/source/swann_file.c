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

#include <swann.h>

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
	  FAILURE("swann_file_init");
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
void
swann_file_init(p_swann_file_t  file,
		char           *pathname)
{
  int status;
  
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
  if (file->state != swann_file_state_initialized)
    FAILURE("invalid file state");

  if (!file->exist)
    FAILURE("file not found");

  status = open(file->pathname,
		O_RDONLY,
		)
#error unimplemented
  LOG_OUT();
}

swann_status_t
swann_file_create(p_swann_file_t file)
{
  LOG_IN();
  LOG_OUT();
}


/*
 * Close
 * -----
 */
swann_status_t
swann_file_close(p_swann_file_t file)
{
  LOG_IN();
  LOG_OUT();
}


/*
 * Destroy
 * -------
 */
swann_status_t
swann_file_destroy(p_swann_file_t file)
{
  LOG_IN();
  LOG_OUT();
}


/*
 * Read/Write  (block)
 * -------------------
 */
swann_status_t
swann_file_read_block(p_swann_file_t  file,
		      void           *ptr,
		      size_t          length)
{
  LOG_IN();
  LOG_OUT();
}

swann_status_t
swann_file_write_block(p_swann_file_t file,
		      void           *ptr,
		      size_t          length)
{
  LOG_IN();
  LOG_OUT();
}

#include <swann.h>


