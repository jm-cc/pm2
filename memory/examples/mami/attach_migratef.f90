!!
! PM2: Parallel Multithreaded Machine
! Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 2 of the License, or (at
! your option) any later version.
!
! This program is distributed in the hope that it will be useful, but
! WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! General Public License for more details.
!


      program attach

      integer*8 size, numthreads
      integer*8 memory_manager
      integer (kind=8) :: self
      integer node

      parameter (size = 64, numthreads=16)

      integer omp_get_thread_num
      external omp_get_thread_num

      integer*8 m3D (size, size, size)
      integer*8 m3D_s, m3D_len, m3D_chk_s
      
      call marcel_init()
      call mami_init(memory_manager)
      call marcel_self(self)

      elt_s = sizeof (m3D (1,1,1))
      m3D_s = sizeof (m3D)
      m3D_chk_s = m3D_s / numthreads
      m3D_len = size*size*size

      write (*,*) "Matrix3D: ", m3D_s

      call mami_task_attach(memory_manager, m3D, m3D_chk_s, self, node, ierr)
      write (*,*) "Node=", node, ", err=", ierr

      node = 0
!      call mami_migrate_on_next_touch(memory_manager, m3D)
      call mami_task_migrate_all(memory_manager, self, node, ierr)

      call mami_locate(memory_manager, m3D, m3D_chk_s, node, ierr)
      write (*,*) "Node=", node, ", err=", ierr

      do k = 1, size
         do j = 1, size
            do i = 1, size
               m3D (i, j, k) = size * size * k + size * j + i
            end do
         end do
      end do

      call mami_task_unattach(memory_manager, m3D, self, ierr)
      call mami_unregister(memory_manager, m3D, ierr)
      call mami_exit(memory_manager)
      call marcel_end()

      end program
