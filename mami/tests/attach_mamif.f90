!!
! MaMI --- NUMA Memory Interface
!
! Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
!
! MaMI is free software; you can redistribute it and/or modify
! it under the terms of the GNU Lesser General Public License as published by
! the Free Software Foundation; either version 2.1 of the License, or (at
! your option) any later version.
!
! MaMI is distributed in the hope that it will be useful, but
! WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
!
! See the GNU Lesser General Public License in COPYING.LGPL for more details.

program launch

  integer node
  integer ierr
  integer*8 size
  integer*8 memory_manager
  integer buffer(100)
  integer*8 self

  call mami_init(memory_manager)
  call marcel_self(self)

  size=100
  call mami_unset_alignment(memory_manager)
  call mami_register(memory_manager, buffer, size, ierr)
  call mami_locate(memory_manager, buffer, size, node, ierr)

  write(*,*) 'node=',node

  buffer(10) = 42

  print *,'buffer(10)=',buffer(10)

  call mami_task_attach(memory_manager, buffer, size, self, node, ierr)
  print *,'node=',node
  call mami_print(memory_manager)

  call mami_task_unattach(memory_manager, buffer, self, ierr)
  call mami_unregister(memory_manager, buffer, ierr)
  call mami_exit(memory_manager)

end program launch
