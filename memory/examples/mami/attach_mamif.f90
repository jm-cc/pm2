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

program launch

  integer node, ierr
  integer memory_manager
  integer buffer(100)
  integer (kind=8) :: self

  call marcel_init()
  call mami_init(memory_manager)
  call marcel_self(self)

  call mami_unset_alignment(memory_manager)
  call mami_register(memory_manager, buffer, 100, ierr)
  call mami_locate(memory_manager, buffer, 100, node, ierr)

  print *,'node=',node

  buffer(10) = 42

  print *,'buffer(10)=',buffer(10)

  call mami_task_attach(memory_manager, buffer, 100, self, node, ierr)
  call mami_print(memory_manager)

  call mami_task_unattach(memory_manager, buffer, self, ierr)
  call mami_unregister(memory_manager, buffer, ierr)
  call mami_exit(memory_manager)
  call marcel_end()

end program launch
